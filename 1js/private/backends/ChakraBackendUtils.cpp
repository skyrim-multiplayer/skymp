#include "ChakraBackendUtils.h"
#include "JsValue.h"

void* ChakraBackendUtils::NativeFunctionImpl(void* callee, bool isConstructorCall,
                                  void** arguments,
                                  unsigned short argumentsCount,
                                  void* callbackState)
  {
    try {
      JsFunctionArgumentsImpl args(arguments, argumentsCount);

      auto f = reinterpret_cast<FunctionT*>(callbackState);
      return (*f)(args).value;
    } catch (std::exception& e) {
      JsValueRef whatStr, err;
      if (JsCreateString(e.what(), strlen(e.what()), &whatStr) == JsNoError &&
          JsCreateError(whatStr, &err) == JsNoError) {
        JsSetException(err);
      }
      return JS_INVALID_REFERENCE;
    }
  }

  std::string ChakraBackendUtils::ConvertJsExceptionToString(JsValueRef exception)
  {
    try {
      auto stack = JsValue(exception).GetProperty("stack").ToString();
      if (stack == "undefined") {
        throw 1;
      }
      return stack;
    } catch (...) {
      std::stringstream ss;
      ss << JsValue(exception).ToString() << std::endl;
      ss << "<unable to get stack>";
      return ss.str();
    }
  }

  std::string ChakraBackendUtils::GetJsExceptionMessage(const char* opName, JsErrorCode ec)
  {
    std::stringstream ss;
    JsValueRef exception;
    if (JsGetAndClearException(&exception) == JsNoError) {
      ss << ConvertJsExceptionToString(exception);
    } else {
      ss << "'" << opName << "' returned error 0x" << std::hex << int(ec);
    }
    return ss.str();
  }