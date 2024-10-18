#pragma once
#include <concepts>
#include <type_traits>

template <typename T>
struct is_integral_constant : std::false_type
{
};

// Match any std::integral_constant, regardless of the value.
template <typename T, T v>
struct is_integral_constant<std::integral_constant<T, v>> : std::true_type
{
};

template <typename T>
concept IntegralConstant = is_integral_constant<std::remove_cv_t<T>>::value;
