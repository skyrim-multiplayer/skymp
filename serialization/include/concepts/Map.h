#pragma once
#include <concepts>
#include <map>

template <typename T>
struct is_std_map : std::false_type
{
};

template <typename K, typename V, typename... Args>
struct is_std_map<std::map<K, V, Args...>> : std::true_type
{
};

template <typename T>
concept Map = is_std_map<T>::value;
