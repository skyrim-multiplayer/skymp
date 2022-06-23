#pragma once

namespace CameraApi {
JsValue WorldPointToScreenPoint(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("worldPointToScreenPoint",
                      JsValue::Function(WorldPointToScreenPoint));
}
}
