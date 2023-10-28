#pragma once
#include <type_traits>

template <typename T>
concept IntegralConstant = requires {
  requires std::is_same_v<
    T, std::integral_constant<typename T::value_type, T::value>>;
};
