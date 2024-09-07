#pragma once
#include <concepts>
#include <nlohmann/json.hpp>
#include <type_traits>

template <typename T>
concept NlohmannJson = std::is_same_v<T, nlohmann::json>;
