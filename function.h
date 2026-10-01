#pragma once
#include <cstdint>
#include <random>
#include <functional>
#include <iostream>
#include <algorithm>
#include <type_traits>

namespace function {

    using Null = std::nullptr_t;

    inline constexpr Null null = nullptr;

    using Timestamp = long long;

    using Magnitude = unsigned long long;

    using Runnable = std::function<void()>;

    template<typename Result>
    using Supplier = std::function<Result()>;

    template<typename Parameter>
    using Consumer = std::function<void(Parameter)>;

    template<typename Parameter1, typename Parameter2>
    using BiConsumer = std::function<void(Parameter1, Parameter2)>;

    template<typename Parameter1, typename Parameter2, typename Parameter3>
    using TriConsumer = std::function<void(Parameter1, Parameter2, Parameter3)>;

    template<typename Parameter, typename Result>
    using Function = std::function<Result(Parameter)>;

    template<typename Parameter1, typename Parameter2, typename Result>
    using BiFunction = std::function<Result(Parameter1, Parameter2)>;

    template<typename Parameter1, typename Parameter2, typename Parameter3, typename Result>
    using TriFunction = std::function<Result(Parameter1, Parameter2, Parameter3)>;

    template<typename Parameter>
    using Predicate = std::function<bool(Parameter)>;

    template<typename Parameter1, typename Parameter2>
    using BiPredicate = std::function<bool(Parameter1, Parameter2)>;

    template<typename Parameter1, typename Parameter2, typename Parameter3>
    using TriPredicate = std::function<bool(Parameter1, Parameter2, Parameter3)>;

    template<typename E>
    using Comparator = std::function<Timestamp(E, E)>;

    template <typename T>
    using Generator = BiConsumer<BiConsumer<T, Timestamp>, BiPredicate<T, Timestamp>>;

	template <typename D>
	D randomly(const D& start, const D& end)
	{
		try
		{
			static std::random_device random;
			static std::mt19937_64 generator(random());

			D maximum = std::max(start, end);
			D minimum = std::min(start, end);

			if constexpr (std::is_integral<D>::value)
			{
				std::uniform_int_distribution<D> distribution(minimum, maximum);
				return distribution(generator);
			}
			else
			{
				std::uniform_real_distribution<D> distribution(minimum, maximum);
				return distribution(generator);
			}
		}
		catch (const std::exception& exception)
		{
			std::cerr << exception.what() << '\n';
			return 0;
		}
	}

	bool randomly()
	{
		try
		{
			static std::random_device device;
			static std::mt19937 generator(device());
			std::bernoulli_distribution distribution(0.5);
			return distribution(generator);
		}
		catch (const std::exception& exception)
		{
			std::cerr << exception.what() << '\n';
		}
		return 0;
	}

};