#pragma once
#include "JsEngine.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace DevApi {
JsValue WriteScript(const JsFunctionArguments& args);
JsValue Require(const JsFunctionArguments& args);

using NativeExportsMap = std::map<std::string, std::function<void(JsValue&)>>;

extern std::shared_ptr<JsEngine>* jsEngine;
extern NativeExportsMap nativeExportsMap;

inline void Register(JsValue& exports, std::shared_ptr<JsEngine>* jsEngine,
                     NativeExportsMap nativeExportsMap)
{
  // Register ma be called multiple times, so we merge maps
  for (auto& p : nativeExportsMap)
    DevApi::nativeExportsMap.insert(p);

  DevApi::jsEngine = jsEngine;

  exports.SetProperty("writeScript", JsValue::Function(WriteScript));
  exports.SetProperty("require", JsValue::Function(Require));
}
}