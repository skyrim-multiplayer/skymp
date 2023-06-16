#pragma once
#include <ostream>
#include <simdjson.h>
#include <stdexcept>
#include <string>
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

class IJsonPointer
{
public:
  virtual ~IJsonPointer() = default;

  virtual const char* GetData() const = 0;
};

inline simdjson::simdjson_result<simdjson::dom::element> GetValueFromJson(
  const simdjson::dom::element& j, const IJsonPointer& jsonPointer)
{
  return j.at_pointer(jsonPointer.GetData());
}

inline simdjson::simdjson_result<simdjson::dom::element> GetValueFromJson(
  const simdjson::dom::element& j, size_t idx)
{
  return j.at(idx);
}

template <class Key, class Value, bool isSimdjsonValue>
struct ReadHelper;

template <class Key, class Value>
struct ReadHelper<Key, Value, true>
{
  static void Read(const simdjson::dom::element& j, Key key, Value* out)
  {
    auto v = GetValueFromJson(j, key);
    if (v.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, v.error());

    *out = v.value();
  }
};

class JsonPointer : public IJsonPointer
{
public:
  JsonPointer(const std::string& key)
  {
    if (!key.empty()) {
      jsonPointer = "/" + key;
    }
  }

  const char* GetData() const override { return jsonPointer.data(); }

  friend std::ostream& operator<<(std::ostream& os,
                                  const JsonPointer& jsonPointer_)
  {
    return os << RemoveFirstSymbol(jsonPointer_.GetData());
  }

private:
  static const char* RemoveFirstSymbol(const char* str)
  {
    return str[0] ? str + 1 : str;
  }

  std::string jsonPointer;
};

template <class Key, class Value>
struct ReadHelper<Key, Value, false>
{
  static void Read(const simdjson::dom::element& j, Key key, Value* out)
  {
    auto v = GetValueFromJson(j, key);
    if (v.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, v.error());

    Value result;
    auto ec = v.get(result);
    if (ec != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, ec);
    *out = result;
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
inline void ReadEx(const simdjson::dom::element& j, const JsonPointer& key,
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
      throw std::runtime_error(                                               \
        std::to_string(v) +                                                   \
        " doesn't match numeric limits of type " #intType);                   \
                                                                              \
    *out = static_cast<intType>(v);                                           \
  }

DeclareReadEx(uint32_t, size_t);
DeclareReadEx(uint32_t, const JsonPointer&);
DeclareReadEx(int32_t, size_t);
DeclareReadEx(int32_t, const JsonPointer&);
#undef DeclareReadEx

template <>
inline void ReadEx<float>(const simdjson::dom::element& j,
                          const JsonPointer& key, float* out)
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
inline void ReadVector(const simdjson::dom::element& j, const JsonPointer& key,
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

// Specialization for std::string
template <>
inline void ReadEx<std::string>(const simdjson::dom::element& j,
                                const JsonPointer& key, std::string* out)
{
  const char* v;
  Read(j, key, &v);
  *out = v;
}

template <>
inline void ReadEx<std::string>(const simdjson::dom::element& j, size_t key,
                                std::string* out)
{
  const char* v;
  Read(j, key, &v);
  *out = v;
}

template <>
inline void ReadEx<uint8_t>(const simdjson::dom::element& j,
                            const JsonPointer& key, uint8_t* out)
{
  uint64_t v;
  Read(j, key, &v);

  if (v > std::numeric_limits<uint8_t>::max())
    throw std::runtime_error(std::to_string(v) +
                             " doesn't match numeric limits of type uint8_t");

  *out = static_cast<uint8_t>(v);
}

template <>
inline void ReadEx<uint8_t>(const simdjson::dom::element& j, size_t key,
                            uint8_t* out)
{
  uint64_t v;
  Read(j, key, &v);

  if (v > std::numeric_limits<uint8_t>::max())
    throw std::runtime_error(std::to_string(v) +
                             " doesn't match numeric limits of type uint8_t");

  *out = static_cast<uint8_t>(v);
}
