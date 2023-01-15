#include "ChakraBackendUtils.h"
#include "JsValue.h"
#include "TaskQueue.h"

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

  void ChakraBackendUtils::OnPromiseContinuation(JsValueRef task, void* state)
  {
    // Equivalent of JsValue::JsValue(JsValueRef *)
    SafeCall(JS_ENGINE_F(JsAddRef), task, nullptr);

    auto taskQueue = reinterpret_cast<Viet::TaskQueue*>(state);

    // RAII doesn't work properly here. That's why we do not just use JsValue.
    // TaskQueue can be destroyed AFTER Chakra deinitialization and then try
    // destroying tasks with JsValue instances captured.
    // Also JsRelease (and JsValue dtor) MUST be called in the Chakra thread.

    // Transfer internal ChakraCore value pointer. We did AddRef so Chakra
    // isn't going to invalidate this pointer.
    taskQueue->AddTask([task] {
      // Equivalent of JsValue::Call({ JsValue::Undefined() })
      JsValueRef undefined, res;
      SafeCall(JS_ENGINE_F(JsGetUndefinedValue), &undefined);
      SafeCall(JS_ENGINE_F(JsCallFunction), task, &undefined, 1,
                        &res);

      // Equivalent of JsValue::~JsValue()
      JsRelease(task, nullptr);
    });
  }

  void ChakraBackendUtils::OnPromiseRejection(JsValueRef promise, JsValueRef reason_,
                                 bool handled, void* state)
  {
    if (handled) {
      // This indicates that failure is handled on the JavaScript side.
      // No sense to do anything.
      return;
    }
    auto q = reinterpret_cast<Viet::TaskQueue*>(state);
    std::stringstream ss;
    auto reason = JsValue(reason_);
    auto stack = reason.GetProperty("stack").ToString();
    ss << "Unhandled promise rejection" << std::endl;
    ss << ((stack == "undefined") ? reason.ToString()
                                  : reason.ToString() + "\n" + stack);
    std::string str = ss.str();

    // Would throw from next TaskQueue::Update call
    q->AddTask([str = std::move(str)] { throw std::runtime_error(str); });
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