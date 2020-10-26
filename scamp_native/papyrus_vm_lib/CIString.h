#pragma once
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

// https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c
struct CICharTraits : public std::char_traits<char>
{
  static bool eq(char c1, char c2) { return toupper(c1) == toupper(c2); }
  static bool ne(char c1, char c2) { return toupper(c1) != toupper(c2); }
  static bool lt(char c1, char c2) { return toupper(c1) < toupper(c2); }
  static int compare(const char* s1, const char* s2, size_t n)
  {
    while (n-- != 0) {
      if (toupper(*s1) < toupper(*s2))
        return -1;
      if (toupper(*s1) > toupper(*s2))
        return 1;
      ++s1;
      ++s2;
    }
    return 0;
  }
  static const char* find(const char* s, int n, char a)
  {
    while (n-- > 0 && toupper(*s) != toupper(a)) {
      ++s;
    }
    return s;
  }
};

using CIString = std::basic_string<char, CICharTraits>;

// https://stackoverflow.com/questions/8627698/case-insensitive-stl-containers-e-g-stdunordered-set

struct CIHash
{
  size_t operator()(const CIString& keyval) const
  {
    std::string keyCopy({ keyval.begin(), keyval.end() });
    std::transform(keyCopy.begin(), keyCopy.end(), keyCopy.begin(), tolower);
    return std::hash<std::string>()(keyCopy);
  }
};

struct CIEqual
{
  bool operator()(const CIString& left, const CIString& right) const
  {
    return left == right;
  }
};

using CISet = std::unordered_set<CIString, CIHash, CIEqual>;

template <class Value>
using CIMap = std::unordered_map<CIString, Value, CIHash, CIEqual>;