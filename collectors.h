#pragma once
#include "collector.h"
#include <memory>
#include <vector>
#include <future>
#include <optional>
#include <string>
#include <set>
#include <list>
#include <deque>
#include <array>
#include <forward_list>
#include <stack>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <initializer_list>
#include <stdexcept>
#include <atomic>
#include <type_traits>
#include <functional>
#include <complex>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <utility>

namespace collectors
{
    const double pi = std::acos(-1);

    template <typename E, typename Predicate>
    auto allMatch(Predicate&& predicate) -> collector::Collector<E, bool, bool>
    {
        return collector::shortable<E, bool, bool>(
            []() -> bool { return true; },
            [](E element, function::Timestamp index, bool accumulator) -> bool { return !accumulator; },
            [predicate = std::forward<Predicate>(predicate)](bool accumulator, E element, function::Timestamp index) -> bool
            {
                if constexpr (std::is_invocable_r_v<bool, Predicate, E, function::Timestamp>)
                {
                    return accumulator && std::invoke(predicate, element, index);
                }
                else if constexpr (std::is_invocable_r_v<bool, Predicate, E>)
                {
                    return accumulator && std::invoke(predicate, element);
                }
                else
                {
                    return false;
                }
            },
            [](bool a, bool b) -> bool { return a && b; },
            [](bool accumulator) -> bool { return accumulator; });
    }

    template <typename E, typename Predicate>
    auto anyMatch(Predicate&& predicate) -> collector::Collector<E, bool, bool>
    {
        return collector::shortable<E, bool, bool>(
            []() -> bool { return false; },
            [](E element, function::Timestamp index, bool accumulator) -> bool { return accumulator; },
            [predicate = std::forward<Predicate>(predicate)](bool accumulator, E element, function::Timestamp index) -> bool
            {
                if constexpr (std::is_invocable_r_v<bool, Predicate, E, function::Timestamp>)
                {
                    return accumulator || std::invoke(predicate, element, index);
                }
                else if constexpr (std::is_invocable_r_v<bool, Predicate, E>)
                {
                    return accumulator || std::invoke(predicate, element);
                }
                else
                {
                    return false;
                }
            },
            [](bool a, bool b) -> bool { return a || b; },
            [](bool accumulator) -> bool { return accumulator; });
    }

    template <typename E, typename Predicate>
    auto noneMatch(Predicate&& predicate) -> collector::Collector<E, bool, bool>
    {
        return collector::shortable<E, bool, bool>(
            []() -> bool { return false; },
            [](E element, function::Timestamp index, bool accumulator) -> bool { return accumulator; },
            [predicate = std::forward<Predicate>(predicate)](bool accumulator, E element, function::Timestamp index) -> bool
            {
                if constexpr (std::is_invocable_r_v<bool, Predicate, E, function::Timestamp>)
                {
                    return accumulator || std::invoke(predicate, element, index);
                }
                else if constexpr (std::is_invocable_r_v<bool, Predicate, E>)
                {
                    return accumulator || std::invoke(predicate, element);
                }
                else
                {
                    return false;
                }
            },
            [](bool a, bool b) -> bool { return a || b; },
            [](bool accumulator) -> bool { return !accumulator; });
    }

    template <typename E, typename Consumer>
    auto forEach(Consumer&& consumer) -> collector::Collector<E, function::Magnitude, function::Magnitude>
    {
        return collector::full<E, function::Magnitude, function::Magnitude>(
            []() -> function::Magnitude { return 0; },
            [consumer](const function::Magnitude& accumulator, const E& element, const function::Timestamp& index) -> function::Magnitude
            {
                if constexpr (std::is_invocable_r_v<void, Consumer, E, function::Timestamp>)
                {
                    std::invoke(consumer, element, index);
                }
                else if constexpr (std::is_invocable_r_v<void, Consumer, E>)
                {
                    std::invoke(consumer, element);
                }
                return accumulator + 1;
            },
            [](const function::Magnitude& a, const function::Magnitude& b) -> function::Magnitude { return a + b; },
            [](const function::Magnitude& initialiserValue) -> function::Magnitude { return initialiserValue; });
    }

    template <typename E>
    auto count() -> collector::Collector<E, function::Magnitude, function::Magnitude>
    {
        return collector::full<E, function::Magnitude, function::Magnitude>(
            []() -> function::Magnitude { return 0LL; },
            [](function::Magnitude accumulatorValue, E element, function::Timestamp index) -> function::Magnitude { return accumulatorValue + 1; },
            [](function::Magnitude a, function::Magnitude b) -> function::Magnitude { return a + b; },
            [](function::Magnitude accumulatorValue) -> function::Magnitude { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto summate() -> collector::Collector<E, D, D>
    {
        return collector::full<E, D, D>(
            []() -> D { return D{}; },
            [](D accumulatorValue, E element, function::Timestamp index) -> D { return accumulatorValue + static_cast<D>(element); },
            [](D a, D b) -> D { return a + b; },
            [](D accumulatorValue) -> D { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto summate(const function::Function<E, D>& mapper) -> collector::Collector<E, D, D>
    {
        return collector::full<E, D, D>(
            []() -> D { return D{}; },
            [mapper](D accumulatorValue, E element, function::Timestamp index) -> D { return accumulatorValue + mapper(element); },
            [](D a, D b) -> D { return a + b; },
            [](D accumulatorValue) -> D { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto average() -> collector::Collector<E, std::pair<D, function::Magnitude>, D>
    {
        return collector::full<E, std::pair<D, function::Magnitude>, D>(
            []() -> std::pair<D, function::Magnitude> { return std::make_pair(D{}, 0); },
            [](std::pair<D, function::Magnitude> accumulatorValue, E element, function::Timestamp index) -> std::pair<D, function::Magnitude>
            {
                D value = static_cast<D>(element);
                return std::make_pair(accumulatorValue.first + value, accumulatorValue.second + 1);
            },
            [](std::pair<D, function::Magnitude> a, std::pair<D, function::Magnitude> b) -> std::pair<D, function::Magnitude>
            {
                return std::make_pair(a.first + b.first, a.second + b.second);
            },
            [](std::pair<D, function::Magnitude> accumulatorValue) -> D
            {
                if (accumulatorValue.second == 0)
                {
                    return D{};
                }
                return accumulatorValue.first / static_cast<D>(accumulatorValue.second);
            });
    }

    template <typename E, typename D>
    auto average(const function::Function<E, D>& mapper) -> collector::Collector<E, std::pair<D, function::Magnitude>, D>
    {
        return collector::full<E, std::pair<D, function::Magnitude>, D>(
            []() -> std::pair<D, function::Magnitude> { return std::make_pair(D{}, 0); },
            [mapper](std::pair<D, function::Magnitude> accumulatorValue, E element, function::Timestamp index) -> std::pair<D, function::Magnitude>
            {
                D value = mapper(element);
                return std::make_pair(accumulatorValue.first + value, accumulatorValue.second + 1);
            },
            [](std::pair<D, function::Magnitude> a, std::pair<D, function::Magnitude> b) -> std::pair<D, function::Magnitude>
            {
                return std::make_pair(a.first + b.first, a.second + b.second);
            },
            [](std::pair<D, function::Magnitude> accumulatorValue) -> D
            {
                if (accumulatorValue.second == 0)
                {
                    return D{};
                }
                return accumulatorValue.first / static_cast<D>(accumulatorValue.second);
            });
    }

    template <typename E, typename D>
    auto range() -> collector::Collector<E, std::pair<D, D>, D>
    {
        return collector::full<E, std::pair<D, D>, D>(
            []() -> std::pair<D, D> { return std::pair<D, D>(D{}, D{}); },
            [](std::pair<D, D> accumulatorValue, E element, function::Timestamp index) -> std::pair<D, D>
            {
                D mapped = static_cast<D>(element);
                if (accumulatorValue.first == D{} && accumulatorValue.second == D{})
                {
                    return std::pair<D, D>(mapped, mapped);
                }
                if (mapped < accumulatorValue.first)
                {
                    return std::pair<D, D>(mapped, accumulatorValue.second);
                }
                if (mapped > accumulatorValue.second)
                {
                    return std::pair<D, D>(accumulatorValue.first, mapped);
                }
                return accumulatorValue;
            },
            [](std::pair<D, D> a, std::pair<D, D> b) -> std::pair<D, D>
            {
                if (a.first == D{} && a.second == D{})
                {
                    return b;
                }
                if (b.first == D{} && b.second == D{})
                {
                    return a;
                }
                return std::pair<D, D>(a.first < b.first ? a.first : b.first, a.second > b.second ? a.second : b.second);
            },
            [](std::pair<D, D> accumulatorValue) -> D
            {
                if (accumulatorValue.first == D{} && accumulatorValue.second == D{})
                {
                    return D{};
                }
                return accumulatorValue.second - accumulatorValue.first;
            });
    }

    template <typename E, typename D>
    auto range(const function::Function<E, D>& mapper) -> collector::Collector<E, std::pair<D, D>, D>
    {
        return collector::full<E, std::pair<D, D>, D>(
            []() -> std::pair<D, D> { return std::pair<D, D>(D{}, D{}); },
            [mapper](std::pair<D, D> accumulatorValue, E element, function::Timestamp index) -> std::pair<D, D>
            {
                D mapped = mapper(element);
                if (accumulatorValue.first == D{} && accumulatorValue.second == D{})
                {
                    return std::pair<D, D>(mapped, mapped);
                }
                if (mapped < accumulatorValue.first)
                {
                    return std::pair<D, D>(mapped, accumulatorValue.second);
                }
                if (mapped > accumulatorValue.second)
                {
                    return std::pair<D, D>(accumulatorValue.first, mapped);
                }
                return accumulatorValue;
            },
            [](std::pair<D, D> a, std::pair<D, D> b) -> std::pair<D, D>
            {
                if (a.first == D{} && a.second == D{})
                {
                    return b;
                }
                if (b.first == D{} && b.second == D{})
                {
                    return a;
                }
                return std::pair<D, D>(a.first < b.first ? a.first : b.first, a.second > b.second ? a.second : b.second);
            },
            [](std::pair<D, D> accumulatorValue) -> D
            {
                if (accumulatorValue.first == D{} && accumulatorValue.second == D{})
                {
                    return D{};
                }
                return accumulatorValue.second - accumulatorValue.first;
            });
    }

    template <typename E, typename D>
    auto minimum() -> collector::Collector<E, std::optional<D>, std::optional<D>>
    {
        return collector::full<E, std::optional<D>, std::optional<D>>(
            []() -> std::optional<D> { return std::nullopt; },
            [](std::optional<D> accumulatorValue, E element, function::Timestamp index) -> std::optional<D>
            {
                D value = static_cast<D>(element);
                if (!accumulatorValue.has_value() || value < accumulatorValue.value())
                {
                    return std::optional<D>(value);
                }
                return accumulatorValue;
            },
            [](std::optional<D> a, std::optional<D> b) -> std::optional<D>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return a.value() < b.value() ? a : b;
            },
            [](std::optional<D> accumulatorValue) -> std::optional<D> { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto minimum(const function::Function<E, D>& mapper) -> collector::Collector<E, std::optional<D>, std::optional<D>>
    {
        return collector::full<E, std::optional<D>, std::optional<D>>(
            []() -> std::optional<D> { return std::nullopt; },
            [mapper](std::optional<D> accumulatorValue, E element, function::Timestamp index) -> std::optional<D>
            {
                D value = mapper(element);
                if (!accumulatorValue.has_value() || value < accumulatorValue.value())
                {
                    return std::optional<D>(value);
                }
                return accumulatorValue;
            },
            [](std::optional<D> a, std::optional<D> b) -> std::optional<D>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return a.value() < b.value() ? a : b;
            },
            [](std::optional<D> accumulatorValue) -> std::optional<D> { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto maximum() -> collector::Collector<E, std::optional<D>, std::optional<D>>
    {
        return collector::full<E, std::optional<D>, std::optional<D>>(
            []() -> std::optional<D> { return std::nullopt; },
            [](std::optional<D> accumulatorValue, E element, function::Timestamp index) -> std::optional<D>
            {
                D value = static_cast<D>(element);
                if (!accumulatorValue.has_value() || value > accumulatorValue.value())
                {
                    return std::optional<D>(value);
                }
                return accumulatorValue;
            },
            [](std::optional<D> a, std::optional<D> b) -> std::optional<D>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return a.value() > b.value() ? a : b;
            },
            [](std::optional<D> accumulatorValue) -> std::optional<D> { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto maximum(const function::Function<E, D>& mapper) -> collector::Collector<E, std::optional<D>, std::optional<D>>
    {
        return collector::full<E, std::optional<D>, std::optional<D>>(
            []() -> std::optional<D> { return std::nullopt; },
            [mapper](std::optional<D> accumulatorValue, E element, function::Timestamp index) -> std::optional<D>
            {
                D value = mapper(element);
                if (!accumulatorValue.has_value() || value > accumulatorValue.value())
                {
                    return std::optional<D>(value);
                }
                return accumulatorValue;
            },
            [](std::optional<D> a, std::optional<D> b) -> std::optional<D>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return a.value() > b.value() ? a : b;
            },
            [](std::optional<D> accumulatorValue) -> std::optional<D> { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto variance() -> collector::Collector<E, std::tuple<D, D, D, function::Magnitude>, D>
    {
        return collector::full<E, std::tuple<D, D, D, function::Magnitude>, D>(
            []() -> std::tuple<D, D, D, function::Magnitude>
            {
                return std::make_tuple(D{}, D{}, D{}, function::Magnitude{ 0 });
            },
            [](std::tuple<D, D, D, function::Magnitude> acc, E element, function::Timestamp index) -> std::tuple<D, D, D, function::Magnitude>
            {
                D value = static_cast<D>(element);
                function::Magnitude count = std::get<3>(acc) + 1;
                D delta = value - std::get<0>(acc);
                D newMean = std::get<0>(acc) + delta / static_cast<D>(count);
                D delta2 = value - newMean;
                D newM2 = std::get<1>(acc) + delta * delta2;
                return std::make_tuple(newMean, newM2, std::get<2>(acc) + value, count);
            },
            [](std::tuple<D, D, D, function::Magnitude> a, std::tuple<D, D, D, function::Magnitude> b) -> std::tuple<D, D, D, function::Magnitude>
            {
                function::Magnitude countA = std::get<3>(a);
                function::Magnitude countB = std::get<3>(b);
                if (countA == 0)
                {
                    return b;
                }
                if (countB == 0)
                {
                    return a;
                }
                D delta = std::get<0>(b) - std::get<0>(a);
                D totalCount = static_cast<D>(countA + countB);
                D newMean = std::get<0>(a) + delta * static_cast<D>(countB) / totalCount;
                D newM2 = std::get<1>(a) + std::get<1>(b) + delta * delta * static_cast<D>(countA) * static_cast<D>(countB) / totalCount;
                return std::make_tuple(newMean, newM2, std::get<2>(a) + std::get<2>(b), countA + countB);
            },
            [](std::tuple<D, D, D, function::Magnitude> acc) -> D
            {
                function::Magnitude count = std::get<3>(acc);
                if (count == 0)
                {
                    return D{};
                }
                return std::get<1>(acc) / static_cast<D>(count);
            });
    }

    template <typename E, typename D>
    auto variance(const function::Function<E, D>& mapper) -> collector::Collector<E, std::tuple<D, D, D, function::Magnitude>, D>
    {
        return collector::full<E, std::tuple<D, D, D, function::Magnitude>, D>(
            []() -> std::tuple<D, D, D, function::Magnitude>
            {
                return std::make_tuple(D{}, D{}, D{}, function::Magnitude{ 0 });
            },
            [mapper](std::tuple<D, D, D, function::Magnitude> acc, E element, function::Timestamp index) -> std::tuple<D, D, D, function::Magnitude>
            {
                D value = mapper(element);
                function::Magnitude count = std::get<3>(acc) + 1;
                D delta = value - std::get<0>(acc);
                D newMean = std::get<0>(acc) + delta / static_cast<D>(count);
                D delta2 = value - newMean;
                D newM2 = std::get<1>(acc) + delta * delta2;
                return std::make_tuple(newMean, newM2, std::get<2>(acc) + value, count);
            },
            [](std::tuple<D, D, D, function::Magnitude> a, std::tuple<D, D, D, function::Magnitude> b) -> std::tuple<D, D, D, function::Magnitude>
            {
                function::Magnitude countA = std::get<3>(a);
                function::Magnitude countB = std::get<3>(b);
                if (countA == 0)
                {
                    return b;
                }
                if (countB == 0)
                {
                    return a;
                }
                D delta = std::get<0>(b) - std::get<0>(a);
                D totalCount = static_cast<D>(countA + countB);
                D newMean = std::get<0>(a) + delta * static_cast<D>(countB) / totalCount;
                D newM2 = std::get<1>(a) + std::get<1>(b) + delta * delta * static_cast<D>(countA) * static_cast<D>(countB) / totalCount;
                return std::make_tuple(newMean, newM2, std::get<2>(a) + std::get<2>(b), countA + countB);
            },
            [](std::tuple<D, D, D, function::Magnitude> acc) -> D
            {
                function::Magnitude count = std::get<3>(acc);
                if (count == 0)
                {
                    return D{};
                }
                return std::get<1>(acc) / static_cast<D>(count);
            });
    }

    template <typename E, typename D>
    auto standardDeviation() -> collector::Collector<E, std::tuple<D, D, D, function::Magnitude>, D>
    {
        return collector::full<E, std::tuple<D, D, D, function::Magnitude>, D>(
            []() -> std::tuple<D, D, D, function::Magnitude>
            {
                return std::make_tuple(D{}, D{}, D{}, function::Magnitude{ 0 });
            },
            [](std::tuple<D, D, D, function::Magnitude> acc, E element, function::Timestamp index) -> std::tuple<D, D, D, function::Magnitude>
            {
                D value = static_cast<D>(element);
                function::Magnitude count = std::get<3>(acc) + 1;
                D delta = value - std::get<0>(acc);
                D newMean = std::get<0>(acc) + delta / static_cast<D>(count);
                D delta2 = value - newMean;
                D newM2 = std::get<1>(acc) + delta * delta2;
                return std::make_tuple(newMean, newM2, std::get<2>(acc) + value, count);
            },
            [](std::tuple<D, D, D, function::Magnitude> a, std::tuple<D, D, D, function::Magnitude> b) -> std::tuple<D, D, D, function::Magnitude>
            {
                function::Magnitude countA = std::get<3>(a);
                function::Magnitude countB = std::get<3>(b);
                if (countA == 0)
                {
                    return b;
                }
                if (countB == 0)
                {
                    return a;
                }
                D delta = std::get<0>(b) - std::get<0>(a);
                D totalCount = static_cast<D>(countA + countB);
                D newMean = std::get<0>(a) + delta * static_cast<D>(countB) / totalCount;
                D newM2 = std::get<1>(a) + std::get<1>(b) + delta * delta * static_cast<D>(countA) * static_cast<D>(countB) / totalCount;
                return std::make_tuple(newMean, newM2, std::get<2>(a) + std::get<2>(b), countA + countB);
            },
            [](std::tuple<D, D, D, function::Magnitude> acc) -> D
            {
                function::Magnitude count = std::get<3>(acc);
                if (count == 0)
                {
                    return D{};
                }
                D variance = std::get<1>(acc) / static_cast<D>(count);
                return static_cast<D>(std::sqrt(static_cast<double>(variance)));
            });
    }

    template <typename E, typename D>
    auto standardDeviation(const function::Function<E, D>& mapper) -> collector::Collector<E, std::tuple<D, D, D, function::Magnitude>, D>
    {
        return collector::full<E, std::tuple<D, D, D, function::Magnitude>, D>(
            []() -> std::tuple<D, D, D, function::Magnitude>
            {
                return std::make_tuple(D{}, D{}, D{}, function::Magnitude{ 0 });
            },
            [mapper](std::tuple<D, D, D, function::Magnitude> acc, E element, function::Timestamp index) -> std::tuple<D, D, D, function::Magnitude>
            {
                D value = mapper(element);
                function::Magnitude count = std::get<3>(acc) + 1;
                D delta = value - std::get<0>(acc);
                D newMean = std::get<0>(acc) + delta / static_cast<D>(count);
                D delta2 = value - newMean;
                D newM2 = std::get<1>(acc) + delta * delta2;
                return std::make_tuple(newMean, newM2, std::get<2>(acc) + value, count);
            },
            [](std::tuple<D, D, D, function::Magnitude> a, std::tuple<D, D, D, function::Magnitude> b) -> std::tuple<D, D, D, function::Magnitude>
            {
                function::Magnitude countA = std::get<3>(a);
                function::Magnitude countB = std::get<3>(b);
                if (countA == 0)
                {
                    return b;
                }
                if (countB == 0)
                {
                    return a;
                }
                D delta = std::get<0>(b) - std::get<0>(a);
                D totalCount = static_cast<D>(countA + countB);
                D newMean = std::get<0>(a) + delta * static_cast<D>(countB) / totalCount;
                D newM2 = std::get<1>(a) + std::get<1>(b) + delta * delta * static_cast<D>(countA) * static_cast<D>(countB) / totalCount;
                return std::make_tuple(newMean, newM2, std::get<2>(a) + std::get<2>(b), countA + countB);
            },
            [](std::tuple<D, D, D, function::Magnitude> acc) -> D
            {
                function::Magnitude count = std::get<3>(acc);
                if (count == 0)
                {
                    return D{};
                }
                D variance = std::get<1>(acc) / static_cast<D>(count);
                return static_cast<D>(std::sqrt(static_cast<double>(variance)));
            });
    }

    template <typename E, typename A, typename R>
    auto collect(function::Supplier<R> initialiser, const std::variant<function::BiFunction<A, E, A>, function::TriFunction<A, E, function::Timestamp, A>>& accumulator, function::BiFunction<A, A, A> combiner, function::Function<A, R> finisher) -> collector::Collector<E, A, R>
    {
        return std::visit([&initialiser, &combiner, &finisher](const auto& accumulation) -> collector::Collector<E, A, R>
            {
                using Accumulation = std::decay_t<decltype(accumulation)>;
                if constexpr (std::is_invocable_v<Accumulation, A, E, function::Timestamp>)
                {
                    return collector::full<E, A, R>(initialiser, accumulation, combiner, finisher);
                }
                else if constexpr (std::is_invocable_v<Accumulation, A, E>)
                {
                    auto wrapped = function::TriFunction<A, E, function::Timestamp, A>(
                        [accumulation](A a, E e, function::Timestamp) -> A { return accumulation(a, e); });
                    return collector::full<E, A, R>(initialiser, wrapped, combiner, finisher);
                }
                else
                {
                    static_assert(sizeof(Accumulation) == 0, "Accumulator must be invocable as either (A, E) -> A or (A, E, Timestamp) -> A");
                }
            },
            accumulator);
    }

    template <typename E, typename A, typename R>
    auto collect(function::Supplier<R> initialiser,  const std::variant<function::BiPredicate<E, function::Timestamp>,  function::TriPredicate<E, function::Timestamp, A>>&interrupter,  const std::variant<function::BiFunction<A, E, A>, function::TriFunction<A, E, function::Timestamp, A>>&accumulator,  function::BiFunction<A, A, A> combiner,  function::Function<A, R> finisher) -> collector::Collector<E, A, R>
    {
        auto normalizedInterrupter = std::visit(
            [](const auto& candidate) -> function::TriPredicate<E, function::Timestamp, A>
            {
                using Candidate = std::decay_t<decltype(candidate)>;
                if constexpr (std::is_invocable_v<Candidate, E, function::Timestamp, A>)
                {
                    return candidate;
                }
                else if constexpr (std::is_invocable_v<Candidate, E, function::Timestamp>)
                {
                    return [candidate](E element, function::Timestamp index, A) -> bool { return candidate(element, index); };
                }
                else
                {
                    static_assert(!std::is_same_v<Candidate, Candidate>, "Interrupter must be invocable as (E, Timestamp) -> bool or (E, Timestamp, A) -> bool");
                }
            }, interrupter);

        auto normalizedAccumulator = std::visit(
            [](const auto& candidate) -> function::TriFunction<A, E, function::Timestamp, A>
            {
                using Candidate = std::decay_t<decltype(candidate)>;
                if constexpr (std::is_invocable_v<Candidate, A, E, function::Timestamp>)
                {
                    return candidate;
                }
                else if constexpr (std::is_invocable_v<Candidate, A, E>)
                {
                    return [candidate](A accumulation, E element, function::Timestamp) -> A { return candidate(accumulation, element); };
                }
                else
                {
                    static_assert(!std::is_same_v<Candidate, Candidate>, "Accumulator must be invocable as (A, E) -> A or (A, E, Timestamp) -> A");
                }
            }, accumulator);

        return collector::shortable<E, A, R>(initialiser, normalizedInterrupter, normalizedAccumulator, combiner, finisher);
    }

    template <typename E>
    auto findAny() -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::shortable<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [](E element, function::Timestamp index, std::optional<E> accumulatorValue) -> bool { return accumulatorValue.has_value(); },
            [](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (function::randomly())
                {
                    return std::optional<E>(element);
                }
                return std::nullopt;
            },
            [](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto findAt(const function::Timestamp& index) -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        if (index < 0LL)
        {
            throw std::invalid_argument("findAt: index must be non-negative, use findNegativeAt for negative index");
        }
        const function::Timestamp target = index;
        return collector::shortable<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [](E element, function::Timestamp index, std::optional<E> accumulatorValue) -> bool { return accumulatorValue.has_value(); },
            [target](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (target == index)
                {
                    return std::optional<E>(element);
                }
                return accumulatorValue;
            },
            [](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto findNegativeAt(const function::Timestamp& index) -> collector::Collector<E, std::map<function::Timestamp, E>, std::optional<E>>
    {
        if (index > -1LL)
        {
            throw std::invalid_argument("findNegativeAt: index must be negative, use findAt for non-negative index");
        }
        return collector::full<E, std::map<function::Timestamp, E>, std::optional<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [index](std::map<function::Timestamp, E> accumulatorValue) -> std::optional<E>
            {
                if (accumulatorValue.empty())
                {
                    return std::nullopt;
                }
                function::Magnitude size = accumulatorValue.size();
                function::Magnitude absIndex = static_cast<function::Magnitude>(std::abs(index));
                function::Magnitude target = (size - (absIndex % size)) % size;
                function::Magnitude counter = 0;
                for (const auto& [key, value] : accumulatorValue)
                {
                    if (counter == target)
                    {
                        return std::optional<E>(value);
                    }
                    counter++;
                }
                return std::nullopt;
            });
    }

    template <typename E>
    auto findFirst() -> collector::Collector<E, std::optional<std::pair<function::Timestamp, E>>, std::optional<E>>
    {
        return collector::full<E, std::optional<std::pair<function::Timestamp, E>>, std::optional<E>>(
            []() -> std::optional<std::pair<function::Timestamp, E>>
            {
                return std::nullopt;
            },
            [](std::optional<std::pair<function::Timestamp, E>> accumulatorValue, E element, function::Timestamp index) -> std::optional<std::pair<function::Timestamp, E>>
            {
                if (!accumulatorValue.has_value() || index < accumulatorValue->first)
                {
                    return std::optional<std::pair<function::Timestamp, E>>(std::make_pair(index, element));
                }
                return accumulatorValue;
            },
            [](std::optional<std::pair<function::Timestamp, E>> a, std::optional<std::pair<function::Timestamp, E>> b) -> std::optional<std::pair<function::Timestamp, E>>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return a->first < b->first ? a : b;
            },
            [](std::optional<std::pair<function::Timestamp, E>> accumulatorValue) -> std::optional<E>
            {
                if (!accumulatorValue.has_value())
                {
                    return std::nullopt;
                }
                return std::optional<E>(accumulatorValue->second);
            });
    }

    template <typename E>
    auto findLast() -> collector::Collector<E, std::map<function::Timestamp, E>, std::optional<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::optional<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::optional<E>
            {
                if (accumulatorValue.empty())
                {
                    return std::nullopt;
                }
                return std::optional<E>(accumulatorValue.rbegin()->second);
            });
    }

    template <typename E>
    auto findMaximum() -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::full<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (accumulatorValue.has_value())
                {
                    if (element > accumulatorValue.value())
                    {
                        return std::optional<E>(element);
                    }
                    return accumulatorValue;
                }
                return std::optional<E>(element);
            },
            [](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    if (b.has_value())
                    {
                        return a.value() > b.value() ? a : b;
                    }
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto findMaximum(const function::Comparator<E>& comparator) -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::full<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [comparator](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (accumulatorValue.has_value())
                {
                    if (comparator(element, accumulatorValue.value()) > 0)
                    {
                        return std::optional<E>(element);
                    }
                    return accumulatorValue;
                }
                return std::optional<E>(element);
            },
            [comparator](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    if (b.has_value())
                    {
                        return comparator(a.value(), b.value()) > 0 ? a : b;
                    }
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto findMinimum() -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::full<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (accumulatorValue.has_value())
                {
                    if (element < accumulatorValue.value())
                    {
                        return std::optional<E>(element);
                    }
                    return accumulatorValue;
                }
                return std::optional<E>(element);
            },
            [](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    if (b.has_value())
                    {
                        return a.value() < b.value() ? a : b;
                    }
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto findMinimum(const function::Comparator<E>& comparator) -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::full<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [comparator](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (accumulatorValue.has_value())
                {
                    if (comparator(element, accumulatorValue.value()) < 0)
                    {
                        return std::optional<E>(element);
                    }
                    return accumulatorValue;
                }
                return std::optional<E>(element);
            },
            [comparator](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (a.has_value())
                {
                    if (b.has_value())
                    {
                        return comparator(a.value(), b.value()) < 0 ? a : b;
                    }
                    return a;
                }
                if (b.has_value())
                {
                    return b;
                }
                return std::nullopt;
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E, typename K, typename KeyExtractor>
    auto group(KeyExtractor&& keyExtractor) -> collector::Collector<E, std::unordered_map<K, std::vector<E>>, std::unordered_map<K, std::vector<E>>>
    {
        return collector::full<E, std::unordered_map<K, std::vector<E>>, std::unordered_map<K, std::vector<E>>>(
            []() -> std::unordered_map<K, std::vector<E>> { return std::unordered_map<K, std::vector<E>>(); },
            [keyExtractor](std::unordered_map<K, std::vector<E>> accumulatorValue, E element, function::Timestamp index) -> std::unordered_map<K, std::vector<E>>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp>)
                {
                    K key = std::invoke(keyExtractor, element, index);
                    accumulatorValue[key].push_back(element);
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E>)
                {
                    K key = std::invoke(keyExtractor, element);
                    accumulatorValue[key].push_back(element);
                }
                return accumulatorValue;
            },
            [](std::unordered_map<K, std::vector<E>> a, std::unordered_map<K, std::vector<E>> b) -> std::unordered_map<K, std::vector<E>>
            {
                for (auto& [key, vec] : b)
                {
                    auto& target = a[key];
                    target.reserve(target.size() + vec.size());
                    target.insert(target.end(), vec.begin(), vec.end());
                }
                return a;
            },
            [](std::unordered_map<K, std::vector<E>> accumulatorValue) -> std::unordered_map<K, std::vector<E>> { return accumulatorValue; });
    }

    template <typename E, typename K, typename V, typename KeyExtractor, typename ValueExtractor>
    auto groupBy(KeyExtractor&& keyExtractor, ValueExtractor&& valueExtractor)
        -> collector::Collector<E, std::unordered_map<K, std::vector<V>>, std::unordered_map<K, std::vector<V>>>
    {
        return collector::full<E, std::unordered_map<K, std::vector<V>>, std::unordered_map<K, std::vector<V>>>(
            []() -> std::unordered_map<K, std::vector<V>> { return std::unordered_map<K, std::vector<V>>(); },
            [keyExtractor, valueExtractor](std::unordered_map<K, std::vector<V>> accumulatorValue, E element, function::Timestamp index) -> std::unordered_map<K, std::vector<V>>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp>)
                {
                    K key = std::invoke(keyExtractor, element, index);
                    if constexpr (std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element, index));
                    }
                    else if constexpr (std::is_invocable_r_v<V, ValueExtractor, E>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element));
                    }
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E>)
                {
                    K key = std::invoke(keyExtractor, element);
                    if constexpr (std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element, index));
                    }
                    else if constexpr (std::is_invocable_r_v<V, ValueExtractor, E>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element));
                    }
                }
                return accumulatorValue;
            },
            [](std::unordered_map<K, std::vector<V>> a, std::unordered_map<K, std::vector<V>> b) -> std::unordered_map<K, std::vector<V>>
            {
                for (auto& [key, vec] : b)
                {
                    auto& target = a[key];
                    target.reserve(target.size() + vec.size());
                    target.insert(target.end(), vec.begin(), vec.end());
                }
                return a;
            },
            [](std::unordered_map<K, std::vector<V>> accumulatorValue) -> std::unordered_map<K, std::vector<V>> { return accumulatorValue; });
    }

    template <typename E>
    auto frequency() -> collector::Collector<E, std::pair<std::unordered_map<E, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>, function::Timestamp>, std::map<E, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>>
    {
        using InnerMap = std::unordered_map<E, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>;
        using AccumulatorType = std::pair<InnerMap, function::Timestamp>;
        using ResultMap = std::map<E, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>;
        return collector::full<E, AccumulatorType, ResultMap>(
            []() -> AccumulatorType
            {
                return AccumulatorType(InnerMap(), 0LL);
            },
            [](AccumulatorType accumulatorValue, E element, function::Timestamp index) -> AccumulatorType
            {
                InnerMap& map = accumulatorValue.first;
                std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& pair = map[element];
                pair.first.push_back(std::complex<double>(static_cast<double>(index), 1.0));
                accumulatorValue.second = index;
                return accumulatorValue;
            },
            [](AccumulatorType a, AccumulatorType b) -> AccumulatorType
            {
                InnerMap& mapA = a.first;
                const InnerMap& mapB = b.first;
                for (auto& entry : mapB)
                {
                    std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& target = mapA[entry.first];
                    target.first.insert(target.first.end(), entry.second.first.begin(), entry.second.first.end());
                }
                if (b.second > a.second)
                {
                    a.second = b.second;
                }
                return a;
            },
            [](AccumulatorType accumulatorValue) -> ResultMap
            {
                ResultMap result;
                InnerMap& map = accumulatorValue.first;
                function::Timestamp totalLength = accumulatorValue.second + 1;
                for (auto& entry : map)
                {
                    std::size_t totalCount = entry.second.first.size();
                    std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& target = result[entry.first];
                    target.first = std::move(entry.second.first);
                    target.second.reserve(totalCount);
                    for (std::size_t i = 0; i < totalCount; i++)
                    {
                        function::Timestamp position = static_cast<function::Timestamp>(target.first[i].real());
                        target.second.push_back(std::complex<double>(static_cast<double>(position), static_cast<double>(totalLength)));
                    }
                }
                return result;
            });
    }

    template <typename E, typename D>
    auto frequency(const function::Function<E, D>& mapper) -> collector::Collector<E, std::pair<std::unordered_map<D, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>, function::Timestamp>, std::map<D, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>>
    {
        using InnerMap = std::unordered_map<D, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>;
        using AccumulatorType = std::pair<InnerMap, function::Timestamp>;
        using ResultMap = std::map<D, std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>>;
        return collector::full<E, AccumulatorType, ResultMap>(
            []() -> AccumulatorType
            {
                return AccumulatorType(InnerMap(), 0LL);
            },
            [mapper](AccumulatorType accumulatorValue, E element, function::Timestamp index) -> AccumulatorType
            {
                InnerMap& map = accumulatorValue.first;
                D key = mapper(element);
                std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& pair = map[key];
                pair.first.push_back(std::complex<double>(static_cast<double>(index), 1.0));
                accumulatorValue.second = index;
                return accumulatorValue;
            },
            [](AccumulatorType a, AccumulatorType b) -> AccumulatorType
            {
                InnerMap& mapA = a.first;
                const InnerMap& mapB = b.first;
                for (auto& entry : mapB)
                {
                    std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& target = mapA[entry.first];
                    target.first.insert(target.first.end(), entry.second.first.begin(), entry.second.first.end());
                }
                if (b.second > a.second)
                {
                    a.second = b.second;
                }
                return a;
            },
            [](AccumulatorType accumulatorValue) -> ResultMap
            {
                ResultMap result;
                InnerMap& map = accumulatorValue.first;
                function::Timestamp totalLength = accumulatorValue.second + 1;
                for (auto& entry : map)
                {
                    std::size_t totalCount = entry.second.first.size();
                    std::pair<std::vector<std::complex<double>>, std::vector<std::complex<double>>>& target = result[entry.first];
                    target.first = std::move(entry.second.first);
                    target.second.reserve(totalCount);
                    for (std::size_t i = 0; i < totalCount; i++)
                    {
                        function::Timestamp position = static_cast<function::Timestamp>(target.first[i].real());
                        target.second.push_back(std::complex<double>(static_cast<double>(position), static_cast<double>(totalLength)));
                    }
                }
                return result;
            });
    }

    template <typename E>
    auto distribution() -> collector::Collector<E, std::unordered_map<E, std::vector<function::Timestamp>>, std::map<E, std::complex<double>>>
    {
        using AccumulatorMap = std::unordered_map<E, std::vector<function::Timestamp>>;
        using ResultMap = std::map<E, std::complex<double>>;
        return collector::full<E, AccumulatorMap, ResultMap>(
            []() -> AccumulatorMap
            {
                return AccumulatorMap();
            },
            [](AccumulatorMap accumulatorValue, E element, function::Timestamp index) -> AccumulatorMap
            {
                accumulatorValue[element].push_back(index);
                return accumulatorValue;
            },
            [](AccumulatorMap a, AccumulatorMap b) -> AccumulatorMap
            {
                for (auto& entry : b)
                {
                    std::vector<function::Timestamp>& target = a[entry.first];
                    target.insert(target.end(), entry.second.begin(), entry.second.end());
                }
                return a;
            },
            [](AccumulatorMap accumulatorValue) -> ResultMap
            {
                ResultMap result;
                if (accumulatorValue.empty())
                {
                    return result;
                }
                std::vector<E> elements;
                std::vector<double> positionSums;
                std::vector<double> counts;
                for (auto& entry : accumulatorValue)
                {
                    elements.push_back(entry.first);
                    double positionSum = 0.0;
                    double count = static_cast<double>(entry.second.size());
                    for (function::Timestamp index : entry.second)
                    {
                        positionSum += static_cast<double>(index);
                    }
                    positionSums.push_back(positionSum);
                    counts.push_back(count);
                }
                double modePositionSum = 0.0;
                double modeCount = 0.0;
                {
                    std::map<double, size_t> positionFreq;
                    for (double val : positionSums)
                    {
                        positionFreq[val]++;
                    }
                    size_t maxFreq = 0;
                    for (auto& p : positionFreq)
                    {
                        if (p.second > maxFreq)
                        {
                            maxFreq = p.second;
                            modePositionSum = p.first;
                        }
                    }
                }
                {
                    std::map<double, size_t> countFreq;
                    for (double val : counts)
                    {
                        countFreq[val]++;
                    }
                    size_t maxFreq = 0;
                    for (auto& p : countFreq)
                    {
                        if (p.second > maxFreq)
                        {
                            maxFreq = p.second;
                            modeCount = p.first;
                        }
                    }
                }
                double positionStddev = 0.0;
                double countStddev = 0.0;
                for (size_t i = 0; i < elements.size(); i++)
                {
                    double posDiff = positionSums[i] - modePositionSum;
                    double cntDiff = counts[i] - modeCount;
                    positionStddev += posDiff * posDiff;
                    countStddev += cntDiff * cntDiff;
                }
                positionStddev = std::sqrt(positionStddev / static_cast<double>(elements.size()));
                countStddev = std::sqrt(countStddev / static_cast<double>(elements.size()));
                if (positionStddev < 0.001)
                {
                    positionStddev = 1.0;
                }
                if (countStddev < 0.001)
                {
                    countStddev = 1.0;
                }
                for (size_t i = 0; i < elements.size(); i++)
                {
                    double posScore = (positionSums[i] - modePositionSum) / positionStddev;
                    double cntScore = (counts[i] - modeCount) / countStddev;
                    result[elements[i]] = std::complex<double>(posScore, cntScore);
                }
                return result;
            });
    }

    template <typename E, typename D>
    auto distribution(const function::Function<E, D>& mapper) -> collector::Collector<E, std::unordered_map<D, std::vector<function::Timestamp>>, std::map<D, std::complex<double>>>
    {
        using AccumulatorMap = std::unordered_map<D, std::vector<function::Timestamp>>;
        using ResultMap = std::map<D, std::complex<double>>;
        return collector::full<E, AccumulatorMap, ResultMap>(
            []() -> AccumulatorMap
            {
                return AccumulatorMap();
            },
            [mapper](AccumulatorMap accumulatorValue, E element, function::Timestamp index) -> AccumulatorMap
            {
                accumulatorValue[mapper(element)].push_back(index);
                return accumulatorValue;
            },
            [](AccumulatorMap a, AccumulatorMap b) -> AccumulatorMap
            {
                for (auto& entry : b)
                {
                    std::vector<function::Timestamp>& target = a[entry.first];
                    target.insert(target.end(), entry.second.begin(), entry.second.end());
                }
                return a;
            },
            [](AccumulatorMap accumulatorValue) -> ResultMap
            {
                ResultMap result;
                if (accumulatorValue.empty())
                {
                    return result;
                }
                std::vector<D> elements;
                std::vector<double> positionSums;
                std::vector<double> counts;
                for (auto& entry : accumulatorValue)
                {
                    elements.push_back(entry.first);
                    double positionSum = 0.0;
                    double count = static_cast<double>(entry.second.size());
                    for (function::Timestamp index : entry.second)
                    {
                        positionSum += static_cast<double>(index);
                    }
                    positionSums.push_back(positionSum);
                    counts.push_back(count);
                }
                double modePositionSum = 0.0;
                double modeCount = 0.0;
                {
                    std::map<double, size_t> positionFreq;
                    for (double val : positionSums)
                    {
                        positionFreq[val]++;
                    }
                    size_t maxFreq = 0;
                    for (auto& p : positionFreq)
                    {
                        if (p.second > maxFreq)
                        {
                            maxFreq = p.second;
                            modePositionSum = p.first;
                        }
                    }
                }
                {
                    std::map<double, size_t> countFreq;
                    for (double val : counts)
                    {
                        countFreq[val]++;
                    }
                    size_t maxFreq = 0;
                    for (auto& p : countFreq)
                    {
                        if (p.second > maxFreq)
                        {
                            maxFreq = p.second;
                            modeCount = p.first;
                        }
                    }
                }
                double positionStddev = 0.0;
                double countStddev = 0.0;
                for (size_t i = 0; i < elements.size(); i++)
                {
                    double posDiff = positionSums[i] - modePositionSum;
                    double cntDiff = counts[i] - modeCount;
                    positionStddev += posDiff * posDiff;
                    countStddev += cntDiff * cntDiff;
                }
                positionStddev = std::sqrt(positionStddev / static_cast<double>(elements.size()));
                countStddev = std::sqrt(countStddev / static_cast<double>(elements.size()));
                if (positionStddev < 0.001)
                {
                    positionStddev = 1.0;
                }
                if (countStddev < 0.001)
                {
                    countStddev = 1.0;
                }
                for (size_t i = 0; i < elements.size(); i++)
                {
                    double posScore = (positionSums[i] - modePositionSum) / positionStddev;
                    double cntScore = (counts[i] - modeCount) / countStddev;
                    result[elements[i]] = std::complex<double>(posScore, cntScore);
                }
                return result;
            });
    }

    template <typename E>
    auto partition(const function::Magnitude& size) -> collector::Collector<E, std::map<function::Timestamp, E>, std::vector<std::vector<E>>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::vector<std::vector<E>>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [size](std::map<function::Timestamp, E> accumulatorValue) -> std::vector<std::vector<E>>
            {
                std::vector<std::vector<E>> result;
                if (size <= 1)
                {
                    for (const auto& [key, value] : accumulatorValue)
                    {
                        result.push_back({ value });
                    }
                    return result;
                }
                std::vector<E> chunk;
                for (const auto& [key, value] : accumulatorValue)
                {
                    chunk.push_back(value);
                    if (chunk.size() >= size)
                    {
                        result.push_back(std::move(chunk));
                        chunk = std::vector<E>();
                    }
                }
                if (!chunk.empty())
                {
                    result.push_back(std::move(chunk));
                }
                return result;
            });
    }

    template <typename E, typename KeyExtractor>
    auto partitionBy(KeyExtractor&& keyExtractor) -> collector::Collector<E, std::map<function::Timestamp, std::vector<E>>, std::vector<std::vector<E>>>
    {
        return collector::full<E, std::map<function::Timestamp, std::vector<E>>, std::vector<std::vector<E>>>(
            []() -> std::map<function::Timestamp, std::vector<E>> { return std::map<function::Timestamp, std::vector<E>>(); },
            [keyExtractor](std::map<function::Timestamp, std::vector<E>> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, std::vector<E>>
            {
                if constexpr (std::is_invocable_r_v<function::Timestamp, KeyExtractor, E, function::Timestamp>)
                {
                    accumulatorValue[std::invoke(keyExtractor, element, index)].push_back(element);
                }
                else if constexpr (std::is_invocable_r_v<function::Timestamp, KeyExtractor, E>)
                {
                    accumulatorValue[std::invoke(keyExtractor, element)].push_back(element);
                }
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, std::vector<E>> a, std::map<function::Timestamp, std::vector<E>> b) -> std::map<function::Timestamp, std::vector<E>>
            {
                for (auto& [key, vec] : b)
                {
                    for (E& element : vec)
                    {
                        a[key].push_back(std::move(element));
                    }
                }
                return a;
            },
            [](std::map<function::Timestamp, std::vector<E>> accumulatorValue) -> std::vector<std::vector<E>>
            {
                std::vector<std::vector<E>> result;
                result.reserve(accumulatorValue.size());
                for (auto& [key, vec] : accumulatorValue)
                {
                    result.push_back(std::move(vec));
                }
                return result;
            });
    }

    template <typename E, typename V, typename KeyExtractor, typename ValueExtractor>
    auto partitionBy(KeyExtractor&& keyExtractor, ValueExtractor&& valueExtractor)
        -> collector::Collector<E, std::map<function::Timestamp, std::vector<V>>, std::vector<std::vector<V>>>
    {
        return collector::full<E, std::map<function::Timestamp, std::vector<V>>, std::vector<std::vector<V>>>(
            []() -> std::map<function::Timestamp, std::vector<V>> { return std::map<function::Timestamp, std::vector<V>>(); },
            [keyExtractor, valueExtractor](std::map<function::Timestamp, std::vector<V>> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, std::vector<V>>
            {
                if constexpr (std::is_invocable_r_v<function::Timestamp, KeyExtractor, E, function::Timestamp>)
                {
                    function::Timestamp key = std::invoke(keyExtractor, element, index);
                    if constexpr (std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element, index));
                    }
                    else if constexpr (std::is_invocable_r_v<V, ValueExtractor, E>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element));
                    }
                }
                else if constexpr (std::is_invocable_r_v<function::Timestamp, KeyExtractor, E>)
                {
                    function::Timestamp key = std::invoke(keyExtractor, element);
                    if constexpr (std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element, index));
                    }
                    else if constexpr (std::is_invocable_r_v<V, ValueExtractor, E>)
                    {
                        accumulatorValue[key].push_back(std::invoke(valueExtractor, element));
                    }
                }
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, std::vector<V>> a, std::map<function::Timestamp, std::vector<V>> b) -> std::map<function::Timestamp, std::vector<V>>
            {
                for (auto& [key, vec] : b)
                {
                    for (V& element : vec)
                    {
                        a[key].push_back(std::move(element));
                    }
                }
                return a;
            },
            [](std::map<function::Timestamp, std::vector<V>> accumulatorValue) -> std::vector<std::vector<V>>
            {
                std::vector<std::vector<V>> result;
                result.reserve(accumulatorValue.size());
                for (auto& [key, vec] : accumulatorValue)
                {
                    result.push_back(std::move(vec));
                }
                return result;
            });
    }

    template <typename E, typename D>
    auto median() -> collector::Collector<E, std::vector<D>, std::optional<D>>
    {
        return collector::full<E, std::vector<D>, std::optional<D>>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(static_cast<D>(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> std::optional<D>
            {
                if (accumulatorValue.empty())
                {
                    return std::nullopt;
                }
                std::sort(accumulatorValue.begin(), accumulatorValue.end());
                if (accumulatorValue.size() % 2 == 1)
                {
                    return std::optional<D>(accumulatorValue[accumulatorValue.size() / 2]);
                }
                return std::optional<D>((accumulatorValue[accumulatorValue.size() / 2 - 1] + accumulatorValue[accumulatorValue.size() / 2]) / static_cast<D>(2));
            });
    }

    template <typename E, typename D>
    auto median(const function::Function<E, D>& mapper) -> collector::Collector<E, std::vector<D>, std::optional<D>>
    {
        return collector::full<E, std::vector<D>, std::optional<D>>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [mapper](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(mapper(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> std::optional<D>
            {
                if (accumulatorValue.empty())
                {
                    return std::nullopt;
                }
                std::sort(accumulatorValue.begin(), accumulatorValue.end());
                if (accumulatorValue.size() % 2 == 1)
                {
                    return std::optional<D>(accumulatorValue[accumulatorValue.size() / 2]);
                }
                return std::optional<D>((accumulatorValue[accumulatorValue.size() / 2 - 1] + accumulatorValue[accumulatorValue.size() / 2]) / static_cast<D>(2));
            });
    }

    template <typename E>
    auto mode() -> collector::Collector<E, std::unordered_map<E, std::complex<double>>, std::optional<E>>
    {
        return collector::full<E, std::unordered_map<E, std::complex<double>>, std::optional<E>>(
            []() -> std::unordered_map<E, std::complex<double>> { return std::unordered_map<E, std::complex<double>>(); },
            [](std::unordered_map<E, std::complex<double>> accumulatorValue, E element, function::Timestamp index) -> std::unordered_map<E, std::complex<double>>
            {
                double angle = std::fmod(2.0 * pi * static_cast<double>(index), 2.0 * pi);
                accumulatorValue[element] += std::complex<double>(std::cos(angle), std::sin(angle));
                return accumulatorValue;
            },
            [](std::unordered_map<E, std::complex<double>> a, std::unordered_map<E, std::complex<double>> b) -> std::unordered_map<E, std::complex<double>>
            {
                for (auto& [key, value] : b)
                {
                    a[key] += value;
                }
                return a;
            },
            [](std::unordered_map<E, std::complex<double>> accumulatorValue) -> std::optional<E>
            {
                if (accumulatorValue.empty())
                {
                    return std::nullopt;
                }
                auto modeIter = std::max_element(accumulatorValue.begin(), accumulatorValue.end(),
                    [](const auto& a, const auto& b)
                    {
                        return std::abs(a.second) < std::abs(b.second);
                    });
                if (std::abs(modeIter->second) == 0.0)
                {
                    return std::nullopt;
                }
                return std::optional<E>(modeIter->first);
            });
    }

    template <typename E, typename D>
    auto percentile(double p) -> collector::Collector<E, std::vector<D>, std::optional<D>>
    {
        if (p < 0.0 || p > 100.0)
        {
            throw std::invalid_argument("percentile: p must be in range [0.0, 100.0]");
        }
        return collector::full<E, std::vector<D>, std::optional<D>>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [](std::vector<D> acc, E element, function::Timestamp index) -> std::vector<D>
            {
                acc.push_back(static_cast<D>(element));
                return acc;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [p](std::vector<D> acc) -> std::optional<D>
            {
                if (acc.empty())
                {
                    return std::nullopt;
                }
                std::sort(acc.begin(), acc.end());
                double rank = p / 100.0 * static_cast<double>(acc.size() - 1);
                std::size_t lo = static_cast<std::size_t>(std::floor(rank));
                std::size_t hi = static_cast<std::size_t>(std::ceil(rank));
                if (lo == hi)
                {
                    return std::optional<D>(acc[lo]);
                }
                double frac = rank - static_cast<double>(lo);
                return std::optional<D>(acc[lo] + static_cast<D>(frac * static_cast<double>(acc[hi] - acc[lo])));
            });
    }

    template <typename E, typename D>
    auto percentile(double p, const function::Function<E, D>& mapper) -> collector::Collector<E, std::vector<D>, std::optional<D>>
    {
        if (p < 0.0 || p > 100.0)
        {
            throw std::invalid_argument("percentile: p must be in range [0.0, 100.0]");
        }
        return collector::full<E, std::vector<D>, std::optional<D>>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [mapper](std::vector<D> acc, E element, function::Timestamp index) -> std::vector<D>
            {
                acc.push_back(mapper(element));
                return acc;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [p](std::vector<D> acc) -> std::optional<D>
            {
                if (acc.empty())
                {
                    return std::nullopt;
                }
                std::sort(acc.begin(), acc.end());
                double rank = p / 100.0 * static_cast<double>(acc.size() - 1);
                std::size_t lo = static_cast<std::size_t>(std::floor(rank));
                std::size_t hi = static_cast<std::size_t>(std::ceil(rank));
                if (lo == hi)
                {
                    return std::optional<D>(acc[lo]);
                }
                double frac = rank - static_cast<double>(lo);
                return std::optional<D>(acc[lo] + static_cast<D>(frac * static_cast<double>(acc[hi] - acc[lo])));
            });
    }

    template <typename E>
    auto reduce(const function::BiFunction<E, E, E>& reducer) -> collector::Collector<E, std::optional<E>, std::optional<E>>
    {
        return collector::full<E, std::optional<E>, std::optional<E>>(
            []() -> std::optional<E> { return std::nullopt; },
            [reducer](std::optional<E> accumulatorValue, E element, function::Timestamp index) -> std::optional<E>
            {
                if (!accumulatorValue.has_value())
                {
                    return std::optional<E>(element);
                }
                return std::optional<E>(reducer(accumulatorValue.value(), element));
            },
            [reducer](std::optional<E> a, std::optional<E> b) -> std::optional<E>
            {
                if (!a.has_value())
                {
                    return b;
                }
                if (!b.has_value())
                {
                    return a;
                }
                return std::optional<E>(reducer(a.value(), b.value()));
            },
            [](std::optional<E> accumulatorValue) -> std::optional<E> { return accumulatorValue; });
    }

    template <typename E>
    auto reduce(const E& initialiser, const function::BiFunction<E, E, E>& reducer) -> collector::Collector<E, E, E>
    {
        return collector::full<E, E, E>(
            [initialiser]() -> E { return initialiser; },
            [reducer](E accumulatorValue, E element, function::Timestamp index) -> E { return reducer(accumulatorValue, element); },
            [reducer](E a, E b) -> E { return reducer(a, b); },
            [](E accumulatorValue) -> E { return accumulatorValue; });
    }

    template <typename E, typename R>
    auto reduce(const R& initialiser, const function::BiFunction<R, E, R>& reducer, const function::BiFunction<R, R, R>& combiner, const function::Function<R, R>& finisher) -> collector::Collector<E, R, R>
    {
        return collector::full<E, R, R>(
            [initialiser]() -> R { return initialiser; },
            [reducer](R accumulatorValue, E element, function::Timestamp index) -> R { return reducer(accumulatorValue, element); },
            [combiner](R a, R b) -> R { return combiner(a, b); },
            [finisher](R accumulatorValue) -> R { return finisher(accumulatorValue); });
    }

    template <typename E, typename K, typename KeyExtractor>
    auto toMap(KeyExtractor&& keyExtractor) -> collector::Collector<E, std::map<K, std::pair<function::Timestamp, E>>, std::map<K, E>>
    {
        return collector::full<E, std::map<K, std::pair<function::Timestamp, E>>, std::map<K, E>>(
            []() -> std::map<K, std::pair<function::Timestamp, E>>
            {
                return std::map<K, std::pair<function::Timestamp, E>>();
            },
            [keyExtractor](std::map<K, std::pair<function::Timestamp, E>> accumulatorValue, E element, function::Timestamp index) -> std::map<K, std::pair<function::Timestamp, E>>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp>)
                {
                    K key = std::invoke(keyExtractor, element, index);
                    auto it = accumulatorValue.find(key);
                    if (it == accumulatorValue.end() || it->second.first < index)
                    {
                        accumulatorValue[key] = std::make_pair(index, element);
                    }
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E>)
                {
                    K key = std::invoke(keyExtractor, element);
                    auto it = accumulatorValue.find(key);
                    if (it == accumulatorValue.end() || it->second.first < index)
                    {
                        accumulatorValue[key] = std::make_pair(index, element);
                    }
                }
                return accumulatorValue;
            },
            [](std::map<K, std::pair<function::Timestamp, E>> a, std::map<K, std::pair<function::Timestamp, E>> b) -> std::map<K, std::pair<function::Timestamp, E>>
            {
                for (const auto& [key, value] : b)
                {
                    auto it = a.find(key);
                    if (it == a.end() || it->second.first < value.first)
                    {
                        a[key] = value;
                    }
                }
                return a;
            },
            [](std::map<K, std::pair<function::Timestamp, E>> accumulatorValue) -> std::map<K, E>
            {
                std::map<K, E> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result[key] = value.second;
                }
                return result;
            });
    }

    template <typename E, typename K, typename V, typename KeyExtractor, typename ValueExtractor>
    auto toMap(KeyExtractor&& keyExtractor, ValueExtractor&& valueExtractor) -> collector::Collector<E, std::map<K, std::pair<function::Timestamp, V>>, std::map<K, V>>
    {
        return collector::full<E, std::map<K, std::pair<function::Timestamp, V>>, std::map<K, V>>(
            []() -> std::map<K, std::pair<function::Timestamp, V>>
            {
                return std::map<K, std::pair<function::Timestamp, V>>();
            },
            [keyExtractor, valueExtractor](std::map<K, std::pair<function::Timestamp, V>> accumulatorValue, E element, function::Timestamp index) -> std::map<K, std::pair<function::Timestamp, V>>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp> && std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                {
                    K key = std::invoke(keyExtractor, element, index);
                    V value = std::invoke(valueExtractor, element, index);
                    auto it = accumulatorValue.find(key);
                    if (it == accumulatorValue.end() || it->second.first < index)
                    {
                        accumulatorValue[key] = std::make_pair(index, value);
                    }
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E> && std::is_invocable_r_v<V, ValueExtractor, E>)
                {
                    K key = std::invoke(keyExtractor, element);
                    V value = std::invoke(valueExtractor, element);
                    auto it = accumulatorValue.find(key);
                    if (it == accumulatorValue.end() || it->second.first < index)
                    {
                        accumulatorValue[key] = std::make_pair(index, value);
                    }
                }
                return accumulatorValue;
            },
            [](std::map<K, std::pair<function::Timestamp, V>> a, std::map<K, std::pair<function::Timestamp, V>> b) -> std::map<K, std::pair<function::Timestamp, V>>
            {
                for (const auto& [key, value] : b)
                {
                    auto it = a.find(key);
                    if (it == a.end() || it->second.first < value.first)
                    {
                        a[key] = value;
                    }
                }
                return a;
            },
            [](std::map<K, std::pair<function::Timestamp, V>> accumulatorValue) -> std::map<K, V>
            {
                std::map<K, V> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result[key] = value.second;
                }
                return result;
            });
    }

    template <typename E, typename K, typename V>
    auto toUnorderedMap(const function::BiFunction<E, function::Timestamp, K>& keyExtractor, const function::BiFunction<E, function::Timestamp, V>& valueExtractor) -> collector::Collector<E, std::unordered_map<K, V>, std::unordered_map<K, V>>
    {
        return collector::full<E, std::unordered_map<K, V>, std::unordered_map<K, V>>(
            []() -> std::unordered_map<K, V> { return std::unordered_map<K, V>(); },
            [keyExtractor, valueExtractor](std::unordered_map<K, V> accumulatorValue, E element, function::Timestamp index) -> std::unordered_map<K, V>
            {
                accumulatorValue[keyExtractor(element, index)] = valueExtractor(element, index);
                return accumulatorValue;
            },
            [](std::unordered_map<K, V> a, std::unordered_map<K, V> b) -> std::unordered_map<K, V>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::unordered_map<K, V> accumulatorValue) -> std::unordered_map<K, V> { return accumulatorValue; });
    }

    template <typename E>
    auto toVector() -> collector::Collector<E, std::map<function::Timestamp, E>, std::vector<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::vector<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::vector<E>
            {
                std::vector<E> result;
                result.reserve(accumulatorValue.size());
                for (const auto& [key, value] : accumulatorValue)
                {
                    result.push_back(value);
                }
                return result;
            });
    }

    template <typename E>
    auto toList() -> collector::Collector<E, std::map<function::Timestamp, E>, std::list<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::list<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::list<E>
            {
                std::list<E> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result.push_back(value);
                }
                return result;
            });
    }

    template <typename E>
    auto toSet() -> collector::Collector<E, std::set<E>, std::set<E>>
    {
        return collector::full<E, std::set<E>, std::set<E>>(
            []() -> std::set<E> { return std::set<E>(); },
            [](std::set<E> accumulatorValue, E element, function::Timestamp index) -> std::set<E>
            {
                accumulatorValue.insert(element);
                return accumulatorValue;
            },
            [](std::set<E> a, std::set<E> b) -> std::set<E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::set<E> accumulatorValue) -> std::set<E> { return accumulatorValue; });
    }

    template <typename E>
    auto toUnorderedSet() -> collector::Collector<E, std::unordered_set<E>, std::unordered_set<E>>
    {
        return collector::full<E, std::unordered_set<E>, std::unordered_set<E>>(
            []() -> std::unordered_set<E> { return std::unordered_set<E>(); },
            [](std::unordered_set<E> accumulatorValue, E element, function::Timestamp index) -> std::unordered_set<E>
            {
                accumulatorValue.insert(element);
                return accumulatorValue;
            },
            [](std::unordered_set<E> a, std::unordered_set<E> b) -> std::unordered_set<E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::unordered_set<E> accumulatorValue) -> std::unordered_set<E> { return accumulatorValue; });
    }

    template <typename E>
    auto toDeque() -> collector::Collector<E, std::map<function::Timestamp, E>, std::deque<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::deque<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::deque<E>
            {
                std::deque<E> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result.push_back(value);
                }
                return result;
            });
    }

    template <typename E>
    auto toForwardList() -> collector::Collector<E, std::map<function::Timestamp, E>, std::forward_list<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::forward_list<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::forward_list<E>
            {
                std::forward_list<E> result;
                auto it = result.before_begin();
                for (const auto& [key, value] : accumulatorValue)
                {
                    it = result.insert_after(it, value);
                }
                return result;
            });
    }

    template <typename E, std::size_t N>
    auto toArray() -> collector::Collector<E, std::map<function::Timestamp, E>, std::array<E, N>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::array<E, N>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::array<E, N>
            {
                std::array<E, N> result{};
                function::Timestamp counter = 0;
                for (const auto& [key, value] : accumulatorValue)
                {
                    if (counter < N)
                    {
                        result[counter] = value;
                    }
                    counter++;
                }
                return result;
            });
    }

    template <typename E>
    auto toMultiset() -> collector::Collector<E, std::multiset<E>, std::multiset<E>>
    {
        return collector::full<E, std::multiset<E>, std::multiset<E>>(
            []() -> std::multiset<E> { return std::multiset<E>(); },
            [](std::multiset<E> accumulatorValue, E element, function::Timestamp index) -> std::multiset<E>
            {
                accumulatorValue.insert(element);
                return accumulatorValue;
            },
            [](std::multiset<E> a, std::multiset<E> b) -> std::multiset<E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::multiset<E> accumulatorValue) -> std::multiset<E> { return accumulatorValue; });
    }

    template <typename E>
    auto toUnorderedMultiset() -> collector::Collector<E, std::unordered_multiset<E>, std::unordered_multiset<E>>
    {
        return collector::full<E, std::unordered_multiset<E>, std::unordered_multiset<E>>(
            []() -> std::unordered_multiset<E> { return std::unordered_multiset<E>(); },
            [](std::unordered_multiset<E> accumulatorValue, E element, function::Timestamp index) -> std::unordered_multiset<E>
            {
                accumulatorValue.insert(element);
                return accumulatorValue;
            },
            [](std::unordered_multiset<E> a, std::unordered_multiset<E> b) -> std::unordered_multiset<E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::unordered_multiset<E> accumulatorValue) -> std::unordered_multiset<E> { return accumulatorValue; });
    }

    template <typename E, typename K, typename KeyExtractor>
    auto toMultimap(KeyExtractor&& keyExtractor) -> collector::Collector<E, std::multimap<K, E>, std::multimap<K, E>>
    {
        return collector::full<E, std::multimap<K, E>, std::multimap<K, E>>(
            []() -> std::multimap<K, E> { return std::multimap<K, E>(); },
            [keyExtractor](std::multimap<K, E> accumulatorValue, E element, function::Timestamp index) -> std::multimap<K, E>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element, index), element));
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element), element));
                }
                return accumulatorValue;
            },
            [](std::multimap<K, E> a, std::multimap<K, E> b) -> std::multimap<K, E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::multimap<K, E> accumulatorValue) -> std::multimap<K, E> { return accumulatorValue; });
    }

    template <typename E, typename K, typename V, typename KeyExtractor, typename ValueExtractor>
    auto toMultimap(KeyExtractor&& keyExtractor, ValueExtractor&& valueExtractor) -> collector::Collector<E, std::multimap<K, V>, std::multimap<K, V>>
    {
        return collector::full<E, std::multimap<K, V>, std::multimap<K, V>>(
            []() -> std::multimap<K, V> { return std::multimap<K, V>(); },
            [keyExtractor, valueExtractor](std::multimap<K, V> accumulatorValue, E element, function::Timestamp index) -> std::multimap<K, V>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp> && std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element, index), std::invoke(valueExtractor, element, index)));
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E> && std::is_invocable_r_v<V, ValueExtractor, E>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element), std::invoke(valueExtractor, element)));
                }
                return accumulatorValue;
            },
            [](std::multimap<K, V> a, std::multimap<K, V> b) -> std::multimap<K, V>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::multimap<K, V> accumulatorValue) -> std::multimap<K, V> { return accumulatorValue; });
    }

    template <typename E, typename K, typename KeyExtractor>
    auto toUnorderedMultimap(KeyExtractor&& keyExtractor) -> collector::Collector<E, std::unordered_multimap<K, E>, std::unordered_multimap<K, E>>
    {
        return collector::full<E, std::unordered_multimap<K, E>, std::unordered_multimap<K, E>>(
            []() -> std::unordered_multimap<K, E> { return std::unordered_multimap<K, E>(); },
            [keyExtractor](std::unordered_multimap<K, E> accumulatorValue, E element, function::Timestamp index) -> std::unordered_multimap<K, E>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element, index), element));
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element), element));
                }
                return accumulatorValue;
            },
            [](std::unordered_multimap<K, E> a, std::unordered_multimap<K, E> b) -> std::unordered_multimap<K, E>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::unordered_multimap<K, E> accumulatorValue) -> std::unordered_multimap<K, E> { return accumulatorValue; });
    }

    template <typename E, typename K, typename V, typename KeyExtractor, typename ValueExtractor>
    auto toUnorderedMultimap(KeyExtractor&& keyExtractor, ValueExtractor&& valueExtractor) -> collector::Collector<E, std::unordered_multimap<K, V>, std::unordered_multimap<K, V>>
    {
        return collector::full<E, std::unordered_multimap<K, V>, std::unordered_multimap<K, V>>(
            []() -> std::unordered_multimap<K, V> { return std::unordered_multimap<K, V>(); },
            [keyExtractor, valueExtractor](std::unordered_multimap<K, V> accumulatorValue, E element, function::Timestamp index) -> std::unordered_multimap<K, V>
            {
                if constexpr (std::is_invocable_r_v<K, KeyExtractor, E, function::Timestamp> && std::is_invocable_r_v<V, ValueExtractor, E, function::Timestamp>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element, index), std::invoke(valueExtractor, element, index)));
                }
                else if constexpr (std::is_invocable_r_v<K, KeyExtractor, E> && std::is_invocable_r_v<V, ValueExtractor, E>)
                {
                    accumulatorValue.insert(std::make_pair(std::invoke(keyExtractor, element), std::invoke(valueExtractor, element)));
                }
                return accumulatorValue;
            },
            [](std::unordered_multimap<K, V> a, std::unordered_multimap<K, V> b) -> std::unordered_multimap<K, V>
            {
                a.insert(b.begin(), b.end());
                return a;
            },
            [](std::unordered_multimap<K, V> accumulatorValue) -> std::unordered_multimap<K, V> { return accumulatorValue; });
    }

    template <typename E>
    auto toStack() -> collector::Collector<E, std::map<function::Timestamp, E>, std::stack<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::stack<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::stack<E>
            {
                std::stack<E> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result.push(value);
                }
                return result;
            });
    }

    template <typename E>
    auto toQueue() -> collector::Collector<E, std::map<function::Timestamp, E>, std::queue<E>>
    {
        return collector::full<E, std::map<function::Timestamp, E>, std::queue<E>>(
            []() -> std::map<function::Timestamp, E>
            {
                return std::map<function::Timestamp, E>();
            },
            [](std::map<function::Timestamp, E> accumulatorValue, E element, function::Timestamp index) -> std::map<function::Timestamp, E>
            {
                accumulatorValue[index] = element;
                return accumulatorValue;
            },
            [](std::map<function::Timestamp, E> a, std::map<function::Timestamp, E> b) -> std::map<function::Timestamp, E>
            {
                for (const auto& [key, value] : b)
                {
                    a[key] = value;
                }
                return a;
            },
            [](std::map<function::Timestamp, E> accumulatorValue) -> std::queue<E>
            {
                std::queue<E> result;
                for (const auto& [key, value] : accumulatorValue)
                {
                    result.push(value);
                }
                return result;
            });
    }

    template <typename E>
    auto toPriorityQueue() -> collector::Collector<E, std::priority_queue<E>, std::priority_queue<E>>
    {
        return collector::full<E, std::priority_queue<E>, std::priority_queue<E>>(
            []() -> std::priority_queue<E> { return std::priority_queue<E>(); },
            [](std::priority_queue<E> accumulatorValue, E element, function::Timestamp index) -> std::priority_queue<E>
            {
                accumulatorValue.push(element);
                return accumulatorValue;
            },
            [](std::priority_queue<E> a, std::priority_queue<E> b) -> std::priority_queue<E>
            {
                while (!b.empty())
                {
                    a.push(b.top());
                    b.pop();
                }
                return a;
            },
            [](std::priority_queue<E> accumulatorValue) -> std::priority_queue<E> { return accumulatorValue; });
    }

    template <typename E, typename D>
    auto skewness() -> collector::Collector<E, std::vector<D>, D>
    {
        return collector::full<E, std::vector<D>, D>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(static_cast<D>(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> D
            {
                if (accumulatorValue.size() < 3)
                {
                    return D{};
                }
                D n = static_cast<D>(accumulatorValue.size());
                D mean = D{};
                for (const D& val : accumulatorValue)
                {
                    mean += val;
                }
                mean /= n;
                D variance = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    variance += diff * diff;
                }
                variance /= n;
                if (variance == D{})
                {
                    return D{};
                }
                D stdDev = static_cast<D>(std::sqrt(static_cast<double>(variance)));
                D sumOfCubes = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    sumOfCubes += diff * diff * diff;
                }
                return (n / ((n - 1) * (n - 2))) * (sumOfCubes / (stdDev * stdDev * stdDev));
            });
    }

    template <typename E, typename D>
    auto skewness(const function::Function<E, D>& mapper) -> collector::Collector<E, std::vector<D>, D>
    {
        return collector::full<E, std::vector<D>, D>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [mapper](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(mapper(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> D
            {
                if (accumulatorValue.size() < 3)
                {
                    return D{};
                }
                D n = static_cast<D>(accumulatorValue.size());
                D mean = D{};
                for (const D& val : accumulatorValue)
                {
                    mean += val;
                }
                mean /= n;
                D variance = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    variance += diff * diff;
                }
                variance /= n;
                if (variance == D{})
                {
                    return D{};
                }
                D stdDev = static_cast<D>(std::sqrt(static_cast<double>(variance)));
                D sumOfCubes = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    sumOfCubes += diff * diff * diff;
                }
                return (n / ((n - 1) * (n - 2))) * (sumOfCubes / (stdDev * stdDev * stdDev));
            });
    }

    template <typename E, typename D>
    auto kurtosis() -> collector::Collector<E, std::vector<D>, D>
    {
        return collector::full<E, std::vector<D>, D>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(static_cast<D>(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> D
            {
                if (accumulatorValue.size() < 4)
                {
                    return D{};
                }
                D n = static_cast<D>(accumulatorValue.size());
                D mean = D{};
                for (const D& val : accumulatorValue)
                {
                    mean += val;
                }
                mean /= n;
                D variance = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    variance += diff * diff;
                }
                variance /= n;
                if (variance == D{})
                {
                    return D{};
                }
                D sumOfQuads = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    sumOfQuads += diff * diff * diff * diff;
                }
                D s4 = variance * variance;
                D num = n * (n + 1) * (n - 1) * sumOfQuads;
                D denom = (n - 2) * (n - 3) * s4 * n * n;
                if (denom == D{})
                {
                    return D{};
                }
                D kurt = num / denom;
                D adjustment = 3.0 * (n - 1) * (n - 1) / ((n - 2) * (n - 3));
                return kurt - adjustment;
            });
    }

    template <typename E, typename D>
    auto kurtosis(const function::Function<E, D>& mapper) -> collector::Collector<E, std::vector<D>, D>
    {
        return collector::full<E, std::vector<D>, D>(
            []() -> std::vector<D> { return std::vector<D>(); },
            [mapper](std::vector<D> accumulatorValue, E element, function::Timestamp index) -> std::vector<D>
            {
                accumulatorValue.push_back(mapper(element));
                return accumulatorValue;
            },
            [](std::vector<D> a, std::vector<D> b) -> std::vector<D>
            {
                a.insert(a.end(), b.begin(), b.end());
                return a;
            },
            [](std::vector<D> accumulatorValue) -> D
            {
                if (accumulatorValue.size() < 4)
                {
                    return D{};
                }
                D n = static_cast<D>(accumulatorValue.size());
                D mean = D{};
                for (const D& val : accumulatorValue)
                {
                    mean += val;
                }
                mean /= n;
                D variance = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    variance += diff * diff;
                }
                variance /= n;
                if (variance == D{})
                {
                    return D{};
                }
                D sumOfQuads = D{};
                for (const D& val : accumulatorValue)
                {
                    D diff = val - mean;
                    sumOfQuads += diff * diff * diff * diff;
                }
                D s4 = variance * variance;
                D num = n * (n + 1) * (n - 1) * sumOfQuads;
                D denom = (n - 2) * (n - 3) * s4 * n * n;
                if (denom == D{})
                {
                    return D{};
                }
                D kurt = num / denom;
                D adjustment = 3.0 * (n - 1) * (n - 1) / ((n - 2) * (n - 3));
                return kurt - adjustment;
            });
    }
}