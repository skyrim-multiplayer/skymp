#pragma once

namespace ObjectReferenceApi {

JsValue SetCollision(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("setCollision", JsValue::Function(SetCollision));
}
}
