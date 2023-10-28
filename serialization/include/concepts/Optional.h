#pragma once
#include <optional>

template <typename T>
struct is_optional : std::false_type
{
};

template <typename T>
struct is_optional<std::optional<T>> : std::true_type
{
};

template <typename T>
concept Optional = is_optional<T>::value;
