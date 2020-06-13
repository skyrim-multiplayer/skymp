#pragma once
#include <simdjson.h>
#include <stdexcept>

class JsonIndexException : public std::exception
{
public:
  template <class K>
  JsonIndexException(const simdjson::dom::element& j, K key,
                     simdjson::error_code ec)
  {
    std::stringstream ss;
    ss << "Unable to read key '" << key << "' from JSON element '"
       << simdjson::minify(j) << "': " << ec;
    str = ss.str();
  }

  const char* what() const override;

private:
  std::string str;
};

template <class Key, class Value>
void Read(const simdjson::dom::element& j, Key key, Value* out)
{
  auto v = j[key];
  if (v.error() != simdjson::error_code::SUCCESS)
    throw JsonIndexException(j, key, v.error());

  if constexpr (std::is_same<Value, simdjson::dom::element>::value) {
    *out = v.value();
  } else {
    auto res = v.get<Value>();
    if (res.error() != simdjson::error_code::SUCCESS)
      throw JsonIndexException(j, key, res.error());

    *out = res.first;
  }
}