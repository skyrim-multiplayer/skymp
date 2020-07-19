#pragma once
#include <simdjson.h>
#include <stdexcept>
#include <typeinfo>

class JsonIndexException : public std::runtime_error
{
public:
  template <class K>
  JsonIndexException(const simdjson::dom::element& j, K key,
                     simdjson::error_code ec)
    : runtime_error(CreateErrorString(j, key, ec))
  {
  }

private:
  template <class K>
  static std::string CreateErrorString(const simdjson::dom::element& j, K key,
                                       simdjson::error_code ec)
  {
    std::stringstream ss;
    ss << "Unable to read key '" << key << "' from JSON element '"
       << simdjson::minify(j) << "': " << ec;
    return ss.str();
  }
};

template <class Key, class Value, bool AorB>
struct ReadHelper;

template <class Key, class Value>
struct ReadHelper<Key, Value, true>
{
  static void Read(const simdjson::dom::element& j, Key key, Value* out)
  {
    auto v = j.at(key);
    if (v.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, v.error());

    *out = v.value();
  }
};

template <class Key, class Value>
struct ReadHelper<Key, Value, false>
{
  static void Read(const simdjson::dom::element& j, Key key, Value* out)
  {
    auto v = j.at(key);
    if (v.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, v.error());

      // We use different versions of simdjson on linux and windows
#ifdef WIN32
    auto res = v.get<typename Value>();
    if (res.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, res.error());
    *out = res.first;
#else
    Value result;
    auto ec = v.get(result);
    if (ec != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, ec);
    *out = result;
#endif
  }
};

template <class Key, class Value>
inline void Read(const simdjson::dom::element& j, Key key, Value* out)
{
  ReadHelper<Key, Value,
             std::is_same<Value, simdjson::dom::element>::value>::Read(j, key,
                                                                       out);
}

template <class Value>
inline void ReadEx(const simdjson::dom::element& j, const char* key,
                   Value* out)
{
  return Read(j, key, out);
}

template <class Value>
inline void ReadEx(const simdjson::dom::element& j, size_t key, Value* out)
{
  return Read(j, key, out);
}

#define DeclareReadEx(intType, keyType)                                       \
  template <>                                                                 \
  inline void ReadEx<intType>(const simdjson::dom::element& j, keyType key,   \
                              intType* out)                                   \
  {                                                                           \
    int64_t v;                                                                \
    Read(j, key, &v);                                                         \
                                                                              \
    if (v < std::numeric_limits<intType>::min() ||                            \
        v > std::numeric_limits<intType>::max())                              \
      throw std::runtime_error(std::to_string(v) +                            \
                               " doesn't match numeric limits of type " +     \
                               (std::string) typeid(intType).name());         \
                                                                              \
    *out = static_cast<intType>(v);                                           \
  }

DeclareReadEx(uint32_t, size_t);
DeclareReadEx(uint32_t, const char*);
DeclareReadEx(int32_t, size_t);
DeclareReadEx(int32_t, const char*);

template <>
inline void ReadEx<float>(const simdjson::dom::element& j, const char* key,
                          float* out)
{
  double v;
  Read(j, key, &v);
  *out = static_cast<float>(v);
}

template <>
inline void ReadEx<float>(const simdjson::dom::element& j, size_t key,
                          float* out)
{
  double v;
  Read(j, key, &v);
  *out = static_cast<float>(v);
}

template <class Vector>
inline void ReadVector(const simdjson::dom::element& j, const char* key,
                       Vector* out)
{
  Vector res;

  simdjson::dom::element arr;
  Read(j, key, &arr);
  size_t n = arr.operator simdjson::dom::array().size();
  res.resize(n);
  for (size_t i = 0; i < n; ++i) {
    ReadEx(arr, i, &res[i]);
  }

  *out = res; // copying here for exception safity
}