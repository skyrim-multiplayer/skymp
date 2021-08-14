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

  void Then(std::function<JsValue(const JsFunctionArguments&)> onResolve)
  {
    promise.GetProperty("then").Call(
      { promise, JsValue::Function(onResolve) });
  }

  void Catch(std::function<JsValue(const JsFunctionArguments&)> onReject)
  {
    promise.GetProperty("catch").Call(
      { promise, JsValue::Function(onReject) });
  }

private:
  JsValue promise;
};