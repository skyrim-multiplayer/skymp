#pragma once
#include "concepts/Concepts.h"
#include <napi.h>

class NapiOutputArchive final
{
public:
  NapiOutputArchive(Napi::Env& env_)
    : env(env_)

  {
  }

  template <IntegralConstant T>
  NapiOutputArchive& Serialize(const T& integralValue )
  {
    value = T::value; // Q: Will it work?
    return *this;
  }

  template <Arithmetic T>
  NapiOutputArchive& Serialize(const T& numberValue)
  {
    value = Napi::Number( numberValue );
    return *this;
  }

  template <StringLike T>
  NapiOutputArchive& Serialize(const T& stringValue)
  {
    value = Napi::String::From(env, stringValue);
    return *this;
  }

  template <ContainerLike T>
  NapiOutputArchive& Serialize(const T& value)
  {
    auto array = Napi::Array(env, value.size());
    for (size_t idx = 0; idx < value.size(); ++idx) {
      NapiOutputArchive child{ env };
      child.Serialize(value[idx]);
      array.Set(idx, child.value);
    }
    value = array;
    return *this;
  }

  template <NoneOfTheAbove T>
  NapiOutputArchive& Serialize(const T& objectValue)
  {
    auto object = Napi::Object();
    NapiOutputArchive child{ env };
    objectValue.Serialize(child);
    value = child.value;
    return *this;
  }

  Napi::Value value;

private:
  Napi::Env& env;
};
