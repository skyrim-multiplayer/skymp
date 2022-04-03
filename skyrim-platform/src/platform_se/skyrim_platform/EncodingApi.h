#pragma once

namespace EncodingApi {
JsValue EncodeUtf8(const JsFunctionArguments& args);
JsValue DecodeUtf8(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("encodeUtf8", JsValue::Function(EncodeUtf8));
  exports.SetProperty("decodeUtf8", JsValue::Function(DecodeUtf8));
}
}
