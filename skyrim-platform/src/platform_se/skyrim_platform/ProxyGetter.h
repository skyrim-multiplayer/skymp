#pragma once

using ProxyGetterFn =
  std::function<JsValue(const JsValue& origin, const JsValue& keyStr)>;

inline JsValue ProxyGetter(const ProxyGetterFn& f)
{
  return JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
    auto& origin = args[1];
    auto& key = args[2];
    auto originProperty = origin.GetProperty(key);
    if (originProperty.GetType() != JsValue::Type::Undefined)
      return originProperty;
    return key.GetType() != JsValue::Type::String ? JsValue::Undefined()
                                                  : f(origin, key);
  });
}
