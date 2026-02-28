#pragma once
#include "concepts/Concepts.h"
#include <napi.h>

class NapiInputArchive
{

public:
  explicit NapiInputArchive(const Napi::Env& env_, const Napi::Value& value_)
    : env{ env_ }
    , value{ value_ }
  {
  }

  template <IntegralConstant T>
  NapiInputArchive& Serialize(T& output)
  {
    return *this;
  }

  template <Arithmetic T>
  NapiInputArchive& Serialize(T& output)
  {
    output = value.As<T>();
    return *this;
  }

  template <StringLike T>
  NapiInputArchive& Serialize(T& output)
  {
    output = static_cast<std::string>(value.ToString());
    return *this;
  }

  template <ContainerLike T>
  NapiInputArchive& Serialize(T& output)
  {
    const Napi::Array& array = value.As<Napi::Array>();
    for (uint32_t idx = 0; idx < array.Length(); ++idx) {
      NapiInputArchive child{ env, array.Get(idx) };
      child.Serialize(output[idx]);
    }
    return *this;
  }

private:
  const Napi::Env& env;
  const Napi::Value& value;
};
