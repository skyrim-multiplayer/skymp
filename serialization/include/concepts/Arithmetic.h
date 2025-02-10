#pragma once
#include <concepts>
#include <type_traits>

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;
