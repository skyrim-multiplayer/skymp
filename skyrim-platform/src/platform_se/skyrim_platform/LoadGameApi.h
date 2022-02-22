#pragma once

namespace LoadGameApi {
JsValue LoadGame(const JsFunctionArguments& args);

inline void Register(JsValue& exports)
{
  exports.SetProperty("loadGame", JsValue::Function(LoadGame));
}
}
