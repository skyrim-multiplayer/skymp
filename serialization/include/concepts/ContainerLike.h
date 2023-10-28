#pragma once
#include <algorithm>
#include <iterator>
#include <type_traits>

template <typename T>
concept ContainerLike = requires(T a) {
  typename T::value_type;
  typename T::iterator; // added
  requires std::same_as<decltype(std::begin(a)), typename T::iterator>;
  requires std::same_as<decltype(std::end(a)), typename T::iterator>;
};