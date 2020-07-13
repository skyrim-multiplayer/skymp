#pragma once
#include <simdjson.h>
#include <stdexcept>

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
void Read(const simdjson::dom::element& j, Key key, Value* out)
{
  ReadHelper<Key, Value,
             std::is_same<Value, simdjson::dom::element>::value>::Read(j, key,
                                                                       out);
}