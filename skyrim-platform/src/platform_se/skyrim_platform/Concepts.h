#pragma once

// concepts to distinguish what approach we should take to acquire event source
template <class T, class E = T::Event>
concept HasEvent = requires {
                     {
                       T::GetEventSource()
                       } -> std::same_as<RE::BSTEventSource<E>*>;
                   };

template <class T, class E>
concept SingletonSource = requires {
                            {
                              T::GetSingleton()
                              } -> std::convertible_to<RE::BSTEventSource<E>*>;
                          };

// concepts to check for primitive types
template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept Enum = std::is_enum_v<T>;

template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template <typename T>
concept IntegralOrEnum = Integral<T> || Enum<T>;
