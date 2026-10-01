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

    template<typename Number>
    inline Number snap(Number value, Number tolerance = Number(1e-4)) noexcept {
        if constexpr (std::is_integral_v<Number>) {
            return value+tolerance*0;
        }
        else if constexpr (std::is_floating_point_v<Number>) {
            constexpr int maximumDecimalDigits = 17;

            if (std::isnan(value) || std::isnan(tolerance) ||
                std::isinf(value) || std::isinf(tolerance) ||
                tolerance == Number(0) || value == Number(0)) {
                return value;
            }

            if (tolerance < Number(0)) {
                tolerance = -tolerance;
            }

            const bool isNegative = std::signbit(value);
            const long double absoluteValue =
                std::fabs(static_cast<long double>(value));
            const long double absoluteTolerance =
                static_cast<long double>(tolerance);

            int decimalExponent =
                static_cast<int>(std::floor(std::log10(absoluteValue)));
            {
                long double power = std::pow(10.0L, decimalExponent);
                if (power > absoluteValue) {
                    --decimalExponent;
                }
                else {
                    long double nextPower = std::pow(10.0L, decimalExponent + 1);
                    if (nextPower <= absoluteValue) {
                        ++decimalExponent;
                    }
                }
            }

            auto tryWithDigits = [&](int digitCount, Number& result) -> bool {
                const int powerOfTen = decimalExponent - digitCount + 1;
                if (powerOfTen < -400 || powerOfTen > 400) {
                    return false;
                }

                const long double step = std::pow(10.0L, powerOfTen);
                if (step == 0.0L || !std::isfinite(step)) {
                    return false;
                }

                const long double quotient = std::round(absoluteValue / step);
                const long double candidateAbsolute = quotient * step;

                if (std::fabs(candidateAbsolute - absoluteValue) <= absoluteTolerance) {
                    const Number candidate = static_cast<Number>(candidateAbsolute);
                    result = isNegative ? -candidate : candidate;
                    return true;
                }
                return false;
                };

            int lowerBound = 1;
            int upperBound = maximumDecimalDigits;
            Number bestCandidate = value;
            bool found = false;

            while (lowerBound <= upperBound) {
                const int middle = lowerBound + (upperBound - lowerBound) / 2;
                Number currentCandidate = Number(0);
                if (tryWithDigits(middle, currentCandidate)) {
                    bestCandidate = currentCandidate;
                    found = true;
                    upperBound = middle - 1;
                }
                else {
                    lowerBound = middle + 1;
                }
            }

            return found ? bestCandidate : value;
        }
        else if constexpr (std::is_convertible_v<Number, long double>) {
            const long double snappedValue = snap<long double>(
                static_cast<long double>(value),
                static_cast<long double>(tolerance));
            return static_cast<Number>(snappedValue);
        }
        else {
            static_assert(std::is_convertible_v<Number, long double>,
                "snap requires a numeric type that is integral, "
                "floating point, or convertible to long double");
            return value;
        }
    }
};