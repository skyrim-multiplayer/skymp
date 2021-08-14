#pragma once
#include "JsEngine.h"

class JsPromise
{
public:
  // Non-explicit by design
  JsPromise(JsValue promise_)
    : promise(promise_)
  {
    if (promise.GetProperty("then").GetType() == JsValue::Type::Undefined) {
      throw std::runtime_error(
        "Attempt to constuct JsPromise from non-promise JsValue");
    }
  }

  // Non-explicit by design
  operator JsValue() const { return promise; }

  static JsPromise New(const JsValue& resolver)
  {
    thread_local auto g_standardPromise =
      JsValue::GlobalObject().GetProperty("Promise");
    return g_standardPromise.Constructor({ g_standardPromise, resolver });
  }

  void Then(std::function<void(const JsFunctionArguments&)> onResolve)
  {
    promise.GetProperty("then").Call({ promise, MakeVoidFunction(onResolve) });
  }

  void Catch(std::function<void(const JsFunctionArguments&)> onReject)
  {
    promise.GetProperty("catch").Call({ promise, MakeVoidFunction(onReject) });
  }

private:
  JsValue MakeVoidFunction(std::function<void(const JsFunctionArguments&)> f)
  {
    return JsValue::Function([f](auto& args) {
      f(args);
      return JsValue::Undefined();
    });
  }

  JsValue promise;
};