#pragma once
#include "JsEngine.h"

/*namespace SystemPolyfill {
JsValue Register_(const JsFunctionArguments& args);

using RegisterFn =
  std::function<void(const std::string& name, JsValue& exports)>;

extern std::shared_ptr<JsEngine>* engine;
extern RegisterFn registerFn;

inline JsValue Register(std::shared_ptr<JsEngine>* engine_,
                        RegisterFn register_)
{
  engine = engine_;
  registerFn = register_;

  auto exports = JsValue::Object();
  exports.SetProperty("register", JsValue::Function(Register_));
  return exports;
}
}*/