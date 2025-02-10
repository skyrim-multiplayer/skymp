#pragma once
#include <concepts>
#include <cstdint>
#include <string>
#include <type_traits>

template <typename T>
concept StringLike =
  std::is_same_v<T, std::string> || std::is_same_v<T, std::wstring> ||
  std::is_same_v<T, std::u8string> || std::is_same_v<T, std::u16string> ||
  std::is_same_v<T, std::u32string> || std::is_same_v<T, std::string_view> ||
  std::is_same_v<T, std::wstring_view> ||
  std::is_same_v<T, std::u8string_view> ||
  std::is_same_v<T, std::u16string_view> ||
  std::is_same_v<T, std::u32string_view> || std::is_same_v<T, const char*> ||
  std::is_same_v<T, const wchar_t*> || std::is_same_v<T, const char8_t*> ||
  std::is_same_v<T, const char16_t*> || std::is_same_v<T, const char32_t*>;
