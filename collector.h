#pragma once
#include "pool.h"
#include <algorithm>
#include <atomic>
#include <future>
#include <memory>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>
#include "function.h"

namespace collector {

    template <typename A>
    using Initialiser = function::Supplier<A>;

    template <typename E, typename A>
    using Interrupter = std::variant<function::BiPredicate<E, function::Timestamp>, function::TriPredicate<E, function::Timestamp, A>>;

    template <typename A, typename E>
    using Accumulator = std::variant<function::BiFunction<A, E, A>, function::TriFunction<A, E, function::Timestamp, A>>;

    template <typename A>
    using Combiner = function::BiFunction<A, A, A>;

    template <typename A, typename R>
    using Finisher = function::Function<A, R>;

    inline pool::ThreadPool& defaultPool() {
        static pool::ThreadPool pool;
        return pool;
    }

    template<typename ThreadPool, typename Container, typename E, typename A, typename R>
    auto container(Initialiser<A> initialiser, Interrupter<E, A> interrupter, Accumulator<A, E> accumulator, Combiner<A> combiner, Finisher<A, R> finisher)  -> function::TriFunction<const ThreadPool&, const Container&, const function::Magnitude&, std::future<R>>
    {
        using Accumulation = A;
        using Result = R;
        using Element = typename Container::value_type;
        using Index = function::Timestamp;
        using Operators = std::tuple<Initialiser<A>, Interrupter<E, A>, Accumulator<A, E>, Combiner<A>, Finisher<A, R>>;

        auto operators = std::make_shared<Operators>(
            std::move(initialiser),
            std::move(interrupter),
            std::move(accumulator),
            std::move(combiner),
            std::move(finisher));

        return [operators](const ThreadPool& threadPool, const Container& container, const function::Magnitude& batch) -> std::future<Result>
            {
                auto sharedContainer = std::make_shared<const Container>(container);
                auto sharedStop = std::make_shared<std::atomic<bool>>(false);
                const ThreadPool* poolPtr = &threadPool;
                const function::Magnitude localBatch = batch;

                if (localBatch < 2ULL) {
                    return std::async(std::launch::async, [operators, sharedContainer, sharedStop]() -> Result
                        {
                            const Container& data = *sharedContainer;
                            Accumulation accumulation = std::get<Initialiser<A>>(*operators)();
                            Index index = 0;
                            for (const Element& element : data) {
                                if (sharedStop->load(std::memory_order_acquire)) {
                                    break;
                                }
                                const bool stopped = std::visit(
                                    [&accumulation, &element, index](const auto& candidate) -> bool
                                    {
                                        using Candidate = std::decay_t<decltype(candidate)>;
                                        if constexpr (std::is_invocable_r_v<bool, Candidate, Element, Index>) {
                                            return candidate(element, index);
                                        }
                                        else {
                                            return candidate(element, index, accumulation);
                                        }
                                    },
                                    std::get<Interrupter<E, A>>(*operators));
                                if (stopped) {
                                    sharedStop->store(true, std::memory_order_release);
                                    break;
                                }
                                accumulation = std::visit(
                                    [&accumulation, &element, index](const auto& candidate) -> Accumulation
                                    {
                                        using Candidate = std::decay_t<decltype(candidate)>;
                                        if constexpr (std::is_invocable_r_v<Accumulation, Candidate, Accumulation, Element, Index>) {
                                            return candidate(accumulation, element, index);
                                        }
                                        else {
                                            return candidate(accumulation, element);
                                        }
                                    },
                                    std::get<Accumulator<A, E>>(*operators));
                                ++index;
                            }
                            return std::get<Finisher<A, R>>(*operators)(accumulation);
                        });
                }

                return std::async(std::launch::async, [operators, poolPtr, sharedContainer, sharedStop, localBatch]() -> Result
                    {
                        const Container& data = *sharedContainer;
                        const ThreadPool& pool = *poolPtr;
                        const std::size_t size = data.size();

                        if (size == 0ULL) {
                            return std::get<Finisher<A, R>>(*operators)(std::get<Initialiser<A>>(*operators)());
                        }

                        const std::size_t workers = std::max<std::size_t>(
                            1ULL,
                            std::min({ static_cast<std::size_t>(localBatch), pool.size(), size }));

                        struct Assignment {
                            std::size_t beginIndex;
                            std::size_t step;
                            std::size_t size;
                        };

                        std::vector<Assignment> assignments;
                        assignments.reserve(workers);
                        for (std::size_t workerIndex = 0ULL; workerIndex < workers; ++workerIndex) {
                            Assignment assignment;
                            assignment.beginIndex = workerIndex;
                            assignment.step = workers;
                            assignment.size = size;
                            assignments.push_back(assignment);
                        }

                        std::vector<std::future<Accumulation>> futures;
                        futures.reserve(assignments.size());

                        for (const Assignment& assignment : assignments) {
                            futures.push_back(pool.submit([operators, assignment, sharedContainer, sharedStop]() -> Accumulation
                                {
                                    const Container& localData = *sharedContainer;
                                    Accumulation accumulation = std::get<Initialiser<A>>(*operators)();
                                    for (std::size_t rawIndex = assignment.beginIndex; rawIndex < assignment.size; rawIndex += assignment.step) {
                                        if (sharedStop->load(std::memory_order_acquire)) {
                                            break;
                                        }
                                        const Element& element = localData[rawIndex];
                                        const Index index = static_cast<Index>(rawIndex);
                                        const bool stopped = std::visit(
                                            [&accumulation, &element, index](const auto& candidate) -> bool
                                            {
                                                using Candidate = std::decay_t<decltype(candidate)>;
                                                if constexpr (std::is_invocable_r_v<bool, Candidate, Element, Index>) {
                                                    return candidate(element, index);
                                                }
                                                else {
                                                    return candidate(element, index, accumulation);
                                                }
                                            },
                                            std::get<Interrupter<E, A>>(*operators));
                                        if (stopped) {
                                            sharedStop->store(true, std::memory_order_release);
                                            break;
                                        }
                                        accumulation = std::visit(
                                            [&accumulation, &element, index](const auto& candidate) -> Accumulation
                                            {
                                                using Candidate = std::decay_t<decltype(candidate)>;
                                                if constexpr (std::is_invocable_r_v<Accumulation, Candidate, Accumulation, Element, Index>) {
                                                    return candidate(accumulation, element, index);
                                                }
                                                else {
                                                    return candidate(accumulation, element);
                                                }
                                            },
                                            std::get<Accumulator<A, E>>(*operators));
                                    }
                                    return accumulation;
                                }));
                        }

                        Accumulation accumulation = std::get<Initialiser<A>>(*operators)();
                        for (auto& future : futures) {
                            accumulation = std::get<Combiner<A>>(*operators)(accumulation, future.get());
                        }
                        return std::get<Finisher<A, R>>(*operators)(accumulation);
                    });
            };
    }

    template<typename ThreadPool, typename E, typename A, typename R>
    auto generator(Initialiser<A> initialiser, Interrupter<E, A> interrupter, Accumulator<A, E> accumulator, Combiner<A> combiner, Finisher<A, R> finisher)  -> function::TriFunction<const ThreadPool&, const function::Generator<E>&, const function::Magnitude&, std::future<R>>
    {
        using Result = R;
        using Index = function::Timestamp;
        using Container = std::vector<E>;

        auto sharedInterrupter = std::make_shared<Interrupter<E, A>>(interrupter);
        auto sharedInitialiser = std::make_shared<Initialiser<A>>(initialiser);

        auto containerParallel = ::collector::container<ThreadPool, Container, E, A, R>(   std::move(initialiser),     std::move(interrupter),     std::move(accumulator),   std::move(combiner),       std::move(finisher));

        return [containerParallel, sharedInterrupter, sharedInitialiser](const ThreadPool& threadPool, const function::Generator<E>& generator, const function::Magnitude& batch) -> std::future<Result>
            {
                Container container;
                A accumulation = (*sharedInitialiser)();

                function::BiConsumer<E, Index> emit = [&container](E element, Index) {
                    container.push_back(std::move(element));
                    };

                function::BiPredicate<E, Index> interrupt = [&sharedInterrupter, &accumulation](const E& element, Index index) -> bool {
                    return std::visit(
                        [&accumulation, &element, index](const auto& candidate) -> bool {
                            using Candidate = std::decay_t<decltype(candidate)>;
                            if constexpr (std::is_invocable_r_v<bool, Candidate, E, Index>) {
                                return candidate(element, index);
                            }
                            else {
                                return candidate(element, index, accumulation);
                            }
                        },
                        *sharedInterrupter);
                    };

                generator(emit, interrupt);

                return containerParallel(threadPool, container, batch);
            };
    }

    template<typename E, typename A, typename R>
    class Collector {
    protected:
        Initialiser<A> initialiser;
        Interrupter<E, A> interrupter;
        Accumulator<A, E> accumulator;
        Combiner<A> combiner;
        Finisher<A, R> finisher;

    public:
        using Accumulation = A;
        using Result = R;

        Collector(Initialiser<A> initialiser, Interrupter<E, A> interrupter, Accumulator<A, E> accumulator, Combiner<A> combiner, Finisher<A, R> finisher) : initialiser(std::move(initialiser)), interrupter(std::move(interrupter)), accumulator(std::move(accumulator)), combiner(std::move(combiner)), finisher(std::move(finisher)) {}

        template<typename ThreadPool, typename Container>
        auto collect(const ThreadPool& threadPool, const Container& container, const function::Magnitude& batch) const -> std::future<Result>
        {
            return ::collector::container<ThreadPool, Container, E, A, R>(initialiser, interrupter, accumulator, combiner, finisher)(threadPool, container, batch);
        }

        template<typename Container>
        auto collect(const Container& container, const function::Magnitude& batch) const -> Result
        {
            return ::collector::container<pool::ThreadPool, Container, E, A, R>(initialiser, interrupter, accumulator, combiner, finisher)(defaultPool(), container, batch).get();
        }

        template<typename ThreadPool>
        auto collect(const ThreadPool& threadPool, const function::Generator<E>& generator, const function::Magnitude& batch) const -> std::future<Result>
        {
            return ::collector::generator<ThreadPool, E, A, R>(initialiser, interrupter, accumulator, combiner, finisher)(threadPool, generator, batch);
        }

        auto collect(const function::Generator<E>& generator, const function::Magnitude& batch) const -> Result
        {
            return ::collector::generator<pool::ThreadPool, E, A, R>(initialiser, interrupter, accumulator, combiner, finisher)(defaultPool(), generator, batch).get();
        }
    };

    template<typename E, typename A, typename R>
    auto full(Initialiser<A> initialiser, Accumulator<A, E> accumulator, Combiner<A> combiner, Finisher<A, R> finisher) -> Collector<E, A, R>
    {
        function::BiPredicate<E, function::Timestamp> never = [](const E&, const function::Timestamp&) -> bool { return false; };
        return Collector<E, A, R>(std::move(initialiser), Interrupter<E, A>{never}, std::move(accumulator), std::move(combiner), std::move(finisher));
    }

    template<typename E, typename A, typename R>
    auto shortable(Initialiser<A> initialiser, Interrupter<E, A> interrupter, Accumulator<A, E> accumulator, Combiner<A> combiner, Finisher<A, R> finisher) -> Collector<E, A, R>
    {
        return Collector<E, A, R>(std::move(initialiser), std::move(interrupter), std::move(accumulator), std::move(combiner), std::move(finisher));
    }
}