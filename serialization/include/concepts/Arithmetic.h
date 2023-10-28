#pragma once
#include <type_traits>

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;
