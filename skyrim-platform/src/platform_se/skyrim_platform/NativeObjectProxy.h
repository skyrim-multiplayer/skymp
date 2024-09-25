#pragma once

#include "NapiHelper.h"
#include "NativeObject.h"

class NativeObjectProxy
{
public:
  static void Attach(Napi::External<NativeObject>& obj,
                     const std::string& cacheClassName,
                     const Napi::Value& toString, const Napi::Value& toJson);
};
