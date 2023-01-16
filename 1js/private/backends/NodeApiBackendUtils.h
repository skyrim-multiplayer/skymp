#pragma once
#include "CommonBackendUtils.h"
#include <js_native_api.h>
#include <stdexcept>

class NodeApiBackendUtils : public CommonBackendUtils {
public:
  template <class F, class... A>
  static void SafeCall(F func, const char* funcName, napi_env env, A... args)
  {
    auto result = func(env, args...);
    if (result != napi_status::napi_ok) {
      throw std::runtime_error(GetJsExceptionMessage(env, funcName, result));
    }
  }

  static void Finalize(napi_env env, void *finalizeData, void *finalizeHint);

  static std::string GetJsExceptionMessage(napi_env env, const char* operationName, napi_status errorCode);

  static napi_value NativeFunctionImpl(napi_env env, napi_callback_info info);
};
