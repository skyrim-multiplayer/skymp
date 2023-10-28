#pragma once
#include <type_traits>

template <typename T>
concept SerializableStruct = requires(T a, auto& archive) {
  {
    a.serialize(archive)
  } -> std::same_as<void>;
};
