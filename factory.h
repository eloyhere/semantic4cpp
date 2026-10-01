#pragma once
#include "semantic.h"

namespace semantic
{

    template <typename D>
    auto useRange(const D& start, const D& end) -> Semantic<D>
    {
        return Semantic<D>([startValue = std::min(start, end), endValue = std::max(start, end)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (D value = startValue; value < endValue; value++)
            {
                if (interrupt(value, index))
                {
                    break;
                }
                accept(value, index);
                index++;
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRange(const D& start, const D& end, const D& step) -> Semantic<D>
    {
        return Semantic<D>([startValue = start, endValue = end, stepValue = step](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            if (stepValue == D{})
            {
                return;
            }
            function::Timestamp index = 0LL;
            if (stepValue > D{})
            {
                for (D value = startValue; value < endValue; value += stepValue)
                {
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            else
            {
                for (D value = startValue; value > endValue; value += stepValue)
                {
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRangeClosed(const D& start, const D& end) -> Semantic<D>
    {
        return Semantic<D>([startValue = std::min(start, end), endValue = std::max(start, end)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (D value = startValue; value <= endValue; value++)
            {
                if (interrupt(value, index))
                {
                    break;
                }
                accept(value, index);
                index++;
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRangeClosed(const D& start, const D& end, const D& step) -> Semantic<D>
    {
        return Semantic<D>([startValue = start, endValue = end, stepValue = step](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            if (stepValue == D{})
            {
                return;
            }
            function::Timestamp index = 0LL;
            if (stepValue > D{})
            {
                for (D value = startValue; value <= endValue; value += stepValue)
                {
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            else
            {
                for (D value = startValue; value >= endValue; value += stepValue)
                {
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            },
            1ULL);
    }

    template <typename D, typename UnaryFunc, typename = std::enable_if_t<std::is_invocable_r_v<D, UnaryFunc, const D&>>>
    auto useInfinite(const D& seed, UnaryFunc&& generator) -> Semantic<D>
    {
        static_assert(std::is_invocable_r_v<D, UnaryFunc, const D&>, "useInfinite: generator must be callable as D(const D&)");
        return Semantic<D>([seed, generator = std::forward<UnaryFunc>(generator)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            D current = seed;
            function::Timestamp index = 0LL;
            while (true)
            {
                if (interrupt(current, index))
                {
                    break;
                }
                accept(current, index);
                current = generator(current);
                index++;
            }
            },
            1ULL);
    }

    template <typename SupplierFunc, typename D = std::invoke_result_t<SupplierFunc>, typename = std::enable_if_t<std::is_invocable_r_v<D, SupplierFunc>>>
    auto useGenerate(SupplierFunc&& supplier) -> Semantic<D>
    {
        static_assert(std::is_invocable_r_v<D, SupplierFunc>, "useGenerate: supplier must be callable as D()");
        return Semantic<D>([supplier = std::forward<SupplierFunc>(supplier)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            while (true)
            {
                D value = supplier();
                if (interrupt(value, index))
                {
                    break;
                }
                accept(value, index);
                index++;
            }
            },
            1ULL);
    }

    template <typename SupplierFunc, typename D = std::invoke_result_t<SupplierFunc>, typename = std::enable_if_t<std::is_invocable_r_v<D, SupplierFunc>>>
    auto useGenerate(SupplierFunc&& supplier, const function::Magnitude& limit) -> Semantic<D>
    {
        static_assert(std::is_invocable_r_v<D, SupplierFunc>, "useGenerate: supplier must be callable as D()");
        return Semantic<D>([supplier = std::forward<SupplierFunc>(supplier), limit](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            while (index < limit)
            {
                D value = supplier();
                if (interrupt(value, index))
                {
                    break;
                }
                accept(value, index);
                index++;
            }
            },
            1ULL);
    }

    template <typename D, typename UnaryFunc, typename = std::enable_if_t<std::is_invocable_r_v<D, UnaryFunc, const D&>>>
    auto useIterate(const D& seed, UnaryFunc&& generator) -> Semantic<D>
    {
        static_assert(std::is_invocable_r_v<D, UnaryFunc, const D&>, "useIterate: generator must be callable as D(const D&)");
        return Semantic<D>([seed, generator = std::forward<UnaryFunc>(generator)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            D current = seed;
            function::Timestamp index = 0LL;
            while (true)
            {
                if (interrupt(current, index))
                {
                    break;
                }
                accept(current, index);
                current = generator(current);
                index++;
            }
            },
            1ULL);
    }

    template <typename D, typename UnaryFunc, typename = std::enable_if_t<std::is_invocable_r_v<D, UnaryFunc, const D&>>>
    auto useIterate(const D& seed, UnaryFunc&& generator, const function::Magnitude& limit) -> Semantic<D>
    {
        static_assert(std::is_invocable_r_v<D, UnaryFunc, const D&>, "useIterate: generator must be callable as D(const D&)");
        return Semantic<D>([seed, generator = std::forward<UnaryFunc>(generator), limit](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            D current = seed;
            function::Timestamp index = 0LL;
            while (index < limit)
            {
                if (interrupt(current, index))
                {
                    break;
                }
                accept(current, index);
                current = generator(current);
                index++;
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRandom() -> Semantic<D>
    {
        return Semantic<D>([](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            std::random_device device;
            std::mt19937 generator(device());
            std::uniform_int_distribution<D> distribution;
            function::Timestamp index = 0LL;
            while (true)
            {
                D value = distribution(generator);
                if (interrupt(value, index))
                {
                    break;
                }
                accept(value, index);
                index++;
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRandom(const D& min, const D& max) -> Semantic<D>
    {
        return Semantic<D>([minValue = std::min(min, max), maxValue = std::max(min, max)](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            std::random_device device;
            std::mt19937 generator(device());
            if constexpr (std::is_integral_v<D>)
            {
                std::uniform_int_distribution<D> distribution(minValue, maxValue);
                function::Timestamp index = 0LL;
                while (true)
                {
                    D value = distribution(generator);
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            else
            {
                std::uniform_real_distribution<D> distribution(minValue, maxValue);
                function::Timestamp index = 0LL;
                while (true)
                {
                    D value = distribution(generator);
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            },
            1ULL);
    }

    template <typename D>
    auto useRandom(const D& min, const D& max, const function::Magnitude& count) -> Semantic<D>
    {
        return Semantic<D>([minValue = std::min(min, max), maxValue = std::max(min, max), count](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            std::random_device device;
            std::mt19937 generator(device());
            if constexpr (std::is_integral_v<D>)
            {
                std::uniform_int_distribution<D> distribution(minValue, maxValue);
                function::Timestamp index = 0LL;
                while (index < count)
                {
                    D value = distribution(generator);
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            else
            {
                std::uniform_real_distribution<D> distribution(minValue, maxValue);
                function::Timestamp index = 0LL;
                while (index < count)
                {
                    D value = distribution(generator);
                    if (interrupt(value, index))
                    {
                        break;
                    }
                    accept(value, index);
                    index++;
                }
            }
            },
            1ULL);
    }

    template <typename D>
    auto useEmpty() -> Semantic<D>
    {
        return Semantic<D>([](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            return;
            });
    }

    template <typename D>
    auto useOf(D element) -> Semantic<D>
    {
        return Semantic<D>([element](function::BiConsumer<D, function::Timestamp> accept, function::BiPredicate<D, function::Timestamp> interrupt) -> void {
            if (!interrupt(element, 0LL))
            {
                accept(element, 0LL);
            }
            },
            1LL);
    }

    template <typename E>
    auto useOf(E element1, E element2) -> Semantic<E>
    {
        return Semantic<E>([element1, element2](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            if (!interrupt(element1, 0LL))
            {
                accept(element1, 0LL);
            }
            if (!interrupt(element2, 1LL))
            {
                accept(element2, 1LL);
            }
            },
            1LL);
    }

    template <typename E>
    auto useOf(E element1, E element2, E element3) -> Semantic<E>
    {
        return Semantic<E>([element1, element2, element3](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            if (!interrupt(element1, 0LL))
            {
                accept(element1, 0LL);
            }
            if (!interrupt(element2, 1LL))
            {
                accept(element2, 1LL);
            }
            if (!interrupt(element3, 2LL))
            {
                accept(element3, 2LL);
            }
            },
            1LL);
    }

    template <typename Container>
    auto useFrom(Container container) -> Semantic<typename Container::value_type>
    {
        using E = typename Container::value_type;
        return Semantic<E>([elements = std::forward<Container>(container)](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const E& element : elements)
            {
                if (interrupt(element, index))
                {
                    break;
                }
                accept(element, index);
                index++;
            }
            },
            1LL);
    }

    template <typename E>
    auto useFrom(std::initializer_list<E> list) -> Semantic<E>
    {
        return Semantic<E>([elements = std::vector<E>(list)](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const E& element : elements)
            {
                if (interrupt(element, index))
                {
                    break;
                }
                accept(element, index);
                index++;
            }
            },
            1LL);
    }

    template <typename E>
    auto useOf(std::initializer_list<E> elements) -> Semantic<E>
    {
        return Semantic<E>([elements = std::vector<E>(elements)](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const E& element : elements)
            {
                if (interrupt(element, index))
                {
                    break;
                }
                accept(element, index);
                index++;
            }
            },
            1LL);
    }

    template <typename E>
    auto useRepeat(const E& element, const function::Magnitude& count) -> Semantic<E>
    {
        return Semantic<E>([element, count](function::BiConsumer<E, function::Timestamp> accept, function::BiPredicate<E, function::Timestamp> interrupt) -> void {
            for (function::Timestamp index = 0LL; index < count; index++)
            {
                if (interrupt(element, index))
                {
                    break;
                }
                accept(element, index);
            }
            });
    }

    inline auto useBlob(const std::string& text) -> Semantic<char>
    {
        return Semantic<char>([text](function::BiConsumer<char, function::Timestamp> accept, function::BiPredicate<char, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const auto& byte : text)
            {
                if (interrupt(byte, index))
                {
                    break;
                }
                accept(byte, index);
                index++;
            }
            },
            1LL);
    }

    inline auto useBlob(const std::string& text, function::Magnitude start, const function::Magnitude end) -> Semantic<char>
    {
        return Semantic<char>([text, start, end](function::BiConsumer<char, function::Timestamp> accept, function::BiPredicate<char, function::Timestamp> interrupt) -> void {
            function::Magnitude limitedStart = std::max(start, static_cast<function::Magnitude>(0LL));
            function::Magnitude limitedEnd = std::min(end, static_cast<function::Magnitude>(text.size()));
            if (limitedStart < limitedEnd)
            {
                function::Timestamp index = 0LL;
                for (function::Magnitude i = limitedStart; i < limitedEnd; i++)
                {
                    if (interrupt(text[i], index))
                    {
                        break;
                    }
                    accept(text[i], index);
                    index++;
                }
            }
            },
            1LL);
    }

    inline auto useBlob(std::istream& stream) -> Semantic<std::string>
    {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(stream, line))
        {
            lines.push_back(line);
        }
        return Semantic<std::string>([lines = std::move(lines)](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const auto& l : lines)
            {
                if (interrupt(l, index))
                {
                    break;
                }
                accept(l, index);
                index++;
            }
            },
            1LL);
    }

    inline auto useBlob(std::istream& stream, const char& delimiter) -> Semantic<std::string>
    {
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(stream, line, delimiter))
        {
            lines.push_back(line);
        }
        return Semantic<std::string>([lines = std::move(lines)](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            function::Timestamp index = 0LL;
            for (const auto& l : lines)
            {
                if (interrupt(l, index))
                {
                    break;
                }
                accept(l, index);
                index++;
            }
            },
            1LL);
    }

    inline auto useText(const std::string& text) -> Semantic<std::string>
    {
        return Semantic<std::string>([text](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            if (!interrupt(text, 0LL))
            {
                accept(text, 0LL);
            }
            },
            1LL);
    }

    inline auto useText(const std::string& text, const char& delimiter) -> Semantic<std::string>
    {
        return Semantic<std::string>([text, delimiter](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            std::vector<std::string> parts;
            std::istringstream stream(text);
            std::string part;
            while (std::getline(stream, part, delimiter))
            {
                parts.push_back(part);
            }
            for (function::Magnitude i = 0; i < parts.size(); i++)
            {
                if (interrupt(parts[i], i))
                {
                    break;
                }
                accept(parts[i], i);
            }
            },
            1LL);
    }

    inline auto useText(std::istream& stream) -> Semantic<std::string>
    {
        std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        return Semantic<std::string>([content](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            if (!interrupt(content, 0LL))
            {
                accept(content, 0LL);
            }
            },
            1LL);
    }

    inline auto useText(std::istream& stream, const char& delimiter) -> Semantic<std::string>
    {
        std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        std::vector<std::string> parts;
        std::istringstream source(content);
        std::string part;
        while (std::getline(source, part, delimiter))
        {
            parts.push_back(part);
        }
        return Semantic<std::string>([parts = std::move(parts)](function::BiConsumer<std::string, function::Timestamp> accept, function::BiPredicate<std::string, function::Timestamp> interrupt) -> void {
            for (function::Magnitude i = 0; i < parts.size(); i++)
            {
                if (interrupt(parts[i], i))
                {
                    break;
                }
                accept(parts[i], i);
            }
            },
            1LL);
    }

} // namespace semantic