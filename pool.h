#pragma once

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif

#ifdef min
#  undef min
#endif

#ifdef max
#  undef max
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace pool {

    enum class Priority {
        High = 0,
        Medium = 1,
        Low = 2
    };

    enum class Error {
        Continuous,
        FailFast,
        ShutDown
    };

    enum class State {
        Pending,
        Resolved,
        Ejected,
        Rejected
    };

    static constexpr std::size_t LevelCount = 3;
    static constexpr std::size_t BatchSize = 8;

    using Runnable = std::function<void()>;

    struct TaskRejected : std::exception {
        const char* what() const noexcept override {
            return "task rejected: ThreadPool has been interrupted";
        }
    };

    struct TaskEjected : std::exception {
        const char* what() const noexcept override {
            return "task ejected: pool is failing fast";
        }
    };

    struct Task {
        Priority priority = Priority::Medium;
        State state = State::Pending;
        Runnable runnable;
        std::function<void()> reject;
    };

    class ThreadPool {
    public:
        explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency(), Error error = Error::Continuous)    : errorPolicy(error)
        {
            if (threads == 0) {
                threads = 1;
            }
            workers.resize(threads);
            for (std::size_t index = 0; index < threads; ++index) {
                workers[index] = std::thread([this]() { work(); });
            }
        }

        ~ThreadPool() {
            interrupt();
            for (std::thread& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        std::size_t size() const {
            return workers.size();
        }

        Error policy() const {
            return errorPolicy;
        }

        bool interrupted() const noexcept {
            return stop.load(std::memory_order_acquire);
        }

        void interrupt() {
            {
                std::lock_guard<std::mutex> guard(queueMutex);
                stop.store(true, std::memory_order_release);
            }
            ready.notify_all();
        }

        void waitAll() {
            std::unique_lock<std::mutex> lock(queueMutex);
            allDone.wait(lock, [this]() {
                return submitted.load(std::memory_order_relaxed)
                    == finished.load(std::memory_order_relaxed);
                });
        }

        template<typename Callable, typename... Args>
        auto submit(Callable&& callable, Args&&... args) const -> std::future<std::invoke_result_t<Callable, Args...>>
        {
            return submitWithPriority(Priority::Medium,
                std::forward<Callable>(callable),
                std::forward<Args>(args)...);
        }

        template<typename Callable, typename... Args>
        auto submitWithPriority(Priority priority, Callable&& callable, Args&&... args) const -> std::future<std::invoke_result_t<Callable, Args...>>
        {
            using ReturnType = std::invoke_result_t<Callable, Args...>;

            auto promise = std::make_shared<std::promise<ReturnType>>();
            std::future<ReturnType> future = promise->get_future();

            Task task;
            task.priority = priority;
            task.state = State::Pending;
            task.runnable = [callable = std::forward<Callable>(callable),
                ... captured = std::forward<Args>(args),
                promise]() mutable {
                try {
                    if constexpr (std::is_void_v<ReturnType>) {
                        std::invoke(std::move(callable), std::move(captured)...);
                        promise->set_value();
                    }
                    else {
                        promise->set_value(std::invoke(std::move(callable), std::move(captured)...));
                    }
                }
                catch (...) {
                    promise->set_exception(std::current_exception());
                    throw;
                }
                };
            task.reject = [promise]() {
                try {
                    promise->set_exception(std::make_exception_ptr(TaskEjected{}));
                }
                catch (...) {
                }
                };

            {
                std::lock_guard<std::mutex> guard(queueMutex);
                if (stop.load(std::memory_order_acquire)) {
                    promise->set_exception(std::make_exception_ptr(TaskRejected{}));
                    return future;
                }
                const std::size_t level = static_cast<std::size_t>(priority);
                queue[level].push_back(std::move(task));
                submitted.fetch_add(1, std::memory_order_relaxed);
            }
            ready.notify_one();
            return future;
        }

    private:
        bool queueEmpty() const {
            for (const std::deque<Task>& level : queue) {
                if (!level.empty()) {
                    return false;
                }
            }
            return true;
        }

        void ejectAllLocked() {
            for (std::deque<Task>& level : queue) {
                for (Task& task : level) {
                    task.state = State::Ejected;
                    if (task.reject) {
                        task.reject();
                    }
                    finished.fetch_add(1, std::memory_order_relaxed);
                }
                level.clear();
            }
        }

        void handleError() {
            switch (errorPolicy) {
            case Error::Continuous:
                break;

            case Error::FailFast: {
                std::lock_guard<std::mutex> guard(queueMutex);
                ejectAllLocked();
                stop.store(true, std::memory_order_release);
                break;
            }

            case Error::ShutDown: {
                std::lock_guard<std::mutex> guard(queueMutex);
                stop.store(true, std::memory_order_release);
                break;
            }
            }
            ready.notify_all();
            allDone.notify_all();
        }

        void work() {
            std::array<std::deque<Task>, LevelCount> local;

            while (true) {
                bool found = false;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);
                    ready.wait(lock, [this]() {
                        return stop.load(std::memory_order_acquire) || !queueEmpty();
                        });

                    for (std::size_t level = 0; level < LevelCount; ++level) {
                        std::deque<Task>& source = queue[level];
                        std::deque<Task>& target = local[level];
                        while (!source.empty() && target.size() < BatchSize) {
                            target.push_back(std::move(source.front()));
                            source.pop_front();
                        }
                        if (!target.empty()) {
                            found = true;
                        }
                    }

                    if (!found) {
                        if (stop.load(std::memory_order_acquire)) {
                            return;
                        }
                        continue;
                    }
                }

                for (std::size_t level = 0; level < LevelCount; ++level) {
                    for (Task& task : local[level]) {
                        try {
                            task.runnable();
                            task.state = State::Resolved;
                            finished.fetch_add(1, std::memory_order_relaxed);
                        }
                        catch (...) {
                            task.state = State::Resolved;
                            finished.fetch_add(1, std::memory_order_relaxed);
                            handleError();
                        }
                        allDone.notify_all();
                    }
                    local[level].clear();
                }
            }
        }

        mutable std::array<std::deque<Task>, LevelCount> queue;  
        mutable std::mutex queueMutex;
        mutable std::condition_variable ready;
        mutable std::condition_variable allDone;

        std::deque<std::thread> workers;
        mutable std::atomic<std::size_t> submitted{ 0 };
        mutable std::atomic<std::size_t> finished{ 0 };
        std::atomic<bool> stop{ false };
        Error errorPolicy;
    };

    template<typename T>
    class ObjectPool {
    public:
        using Factory = std::function<T* ()>;
        using Cleaner = std::function<void(T&)>;

        explicit ObjectPool(std::size_t capacity,
            Factory factory = {},
            Cleaner cleaner = {})
        {
            if (capacity == 0) {
                throw std::invalid_argument("ObjectPool capacity must be greater than zero");
            }
            Core* raw = CoreCache::acquire();
            raw->reset(capacity, std::move(factory), std::move(cleaner));
            core = std::shared_ptr<Core>(raw, [](Core* c) {
                c->drain();
                CoreCache::release(c);
                });
        }

        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;
        ObjectPool(ObjectPool&&) = delete;
        ObjectPool& operator=(ObjectPool&&) = delete;

        std::shared_ptr<T> acquire() {
            std::unique_lock<std::mutex> lock(core->mutex);
            core->available.wait(lock, [this]() {
                return !core->freeList.empty() || core->created < core->capacity;
                });
            return takeLocked(lock);
        }

        template<typename Rep, typename Period>
        std::shared_ptr<T> acquireFor(const std::chrono::duration<Rep, Period>& timeout) {
            std::unique_lock<std::mutex> lock(core->mutex);
            const bool ready = core->available.wait_for(lock, timeout, [this]() {
                return !core->freeList.empty() || core->created < core->capacity;
                });
            if (!ready) {
                throw std::runtime_error("ObjectPool::acquireFor timed out");
            }
            return takeLocked(lock);
        }

        std::shared_ptr<T> tryAcquire() {
            std::unique_lock<std::mutex> lock(core->mutex);
            if (core->freeList.empty() && core->created >= core->capacity) {
                return nullptr;
            }
            return takeLocked(lock);
        }

        std::size_t capacity() const noexcept {
            return core->capacity;
        }

        std::size_t available() const {
            std::lock_guard<std::mutex> guard(core->mutex);
            return core->freeList.size();
        }

        std::size_t inUse() const {
            std::lock_guard<std::mutex> guard(core->mutex);
            return core->borrowed;
        }

        std::size_t created() const {
            std::lock_guard<std::mutex> guard(core->mutex);
            return core->created;
        }

        void reserve(std::size_t count) {
            std::unique_lock<std::mutex> lock(core->mutex);
            const std::size_t target = (std::min)(count, core->capacity);
            while (core->created < target) {
                ++core->created;
                lock.unlock();
                T* raw = nullptr;
                try {
                    raw = core->factory ? core->factory() : new T();
                }
                catch (...) {
                    lock.lock();
                    --core->created;
                    lock.unlock();
                    core->available.notify_one();
                    throw;
                }
                lock.lock();
                try {
                    core->freeList.push_back(raw);
                }
                catch (...) {
                    --core->created;
                    lock.unlock();
                    try { delete raw; }
                    catch (...) {}
                    core->available.notify_one();
                    throw;
                }
                lock.unlock();
                core->available.notify_one();
                lock.lock();
            }
        }

        void clear() {
            std::lock_guard<std::mutex> guard(core->mutex);
            for (T* ptr : core->freeList) {
                try { delete ptr; }
                catch (...) {}
            }
            core->created -= core->freeList.size();
            core->freeList.clear();
        }

    private:
        struct Core {
            std::size_t capacity = 0;
            Factory factory;
            Cleaner cleaner;
            std::deque<T*> freeList;
            std::size_t created = 0;
            std::size_t borrowed = 0;
            std::mutex mutex;
            std::condition_variable available;

            void reset(std::size_t cap, Factory f, Cleaner c) {
                capacity = cap;
                factory = std::move(f);
                cleaner = std::move(c);
                freeList.clear();
                created = 0;
                borrowed = 0;
            }

            void drain() {
                for (T* ptr : freeList) {
                    try { delete ptr; }
                    catch (...) {}
                }
                freeList.clear();
            }
        };

        struct CoreCache {
            static constexpr std::size_t MaxCached = 64;

            static std::mutex& mutex() {
                static std::mutex m;
                return m;
            }

            static std::deque<Core*>& pool() {
                static std::deque<Core*> p;
                return p;
            }

            static Core* acquire() {
                std::lock_guard<std::mutex> guard(mutex());
                auto& p = pool();
                if (!p.empty()) {
                    Core* c = p.back();
                    p.pop_back();
                    return c;
                }
                return new Core();
            }

            static void release(Core* c) {
                std::lock_guard<std::mutex> guard(mutex());
                auto& p = pool();
                if (p.size() >= MaxCached) {
                    delete c;
                    return;
                }
                p.push_back(c);
            }
        };

        std::shared_ptr<T> takeLocked(std::unique_lock<std::mutex>& lock) {
            T* raw = nullptr;

            if (!core->freeList.empty()) {
                raw = core->freeList.back();
                core->freeList.pop_back();
            }
            else {
                ++core->created;
                lock.unlock();
                try {
                    raw = core->factory ? core->factory() : new T();
                }
                catch (...) {
                    lock.lock();
                    --core->created;
                    lock.unlock();
                    core->available.notify_one();
                    throw;
                }
                lock.lock();
            }

            ++core->borrowed;

            std::shared_ptr<Core> localCore = core;
            return std::shared_ptr<T>(raw, [localCore](T* ptr) {
                if (localCore->cleaner) {
                    try {
                        localCore->cleaner(*ptr);
                    }
                    catch (...) {
                        try { delete ptr; }
                        catch (...) {}
                        std::lock_guard<std::mutex> guard(localCore->mutex);
                        --localCore->created;
                        --localCore->borrowed;
                        localCore->available.notify_one();
                        return;
                    }
                }
                try {
                    std::lock_guard<std::mutex> guard(localCore->mutex);
                    localCore->freeList.push_back(ptr);
                    --localCore->borrowed;
                    localCore->available.notify_one();
                }
                catch (...) {
                    try { delete ptr; }
                    catch (...) {}
                    std::lock_guard<std::mutex> guard(localCore->mutex);
                    --localCore->created;
                    --localCore->borrowed;
                    localCore->available.notify_one();
                }
                });
        }

        std::shared_ptr<Core> core;
    };

    template<typename T>
    auto makeObjectPool(std::size_t capacity) {
        return std::make_unique<ObjectPool<T>>(capacity);
    }

    template<typename T, typename Factory>
    auto makeObjectPool(std::size_t capacity, Factory factory) {
        return std::make_unique<ObjectPool<T>>(capacity, std::forward<Factory>(factory));
    }
}