#pragma once
#include <stdexcept>
#include <ChakraCore.h>
#include "private/JsFunctionArgumentsImpl.h"
#include "FunctionT.h"
#include "CommonBackendUtils.h"

class ChakraBackendUtils : public CommonBackendUtils {
public:
  template <class F, class... A>
  static void SafeCall(F func, const char* funcName, A... args)
  {
    auto result = func(args...);
    if (result != JsErrorCode::JsNoError) {
      throw std::runtime_error(GetJsExceptionMessage(funcName, result));
    }
  }

  static void* NativeFunctionImpl(void* callee, bool isConstructorCall,
                                  void** arguments,
                                  unsigned short argumentsCount,
                                  void* callbackState);

  static void OnPromiseContinuation(JsValueRef task, void* state);

  static void OnPromiseRejection(JsValueRef promise, JsValueRef reason_,
                                 bool handled, void* state);

  static std::string ConvertJsExceptionToString(JsValueRef exception);

  static std::string GetJsExceptionMessage(const char* opName, JsErrorCode ec);
};
