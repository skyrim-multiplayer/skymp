#pragma once
#include "JsEngine.h"

#include <functional>
#include <map>
#include <memory>
#include <string>

namespace DevApi {
JsValue Require(const JsFunctionArguments& args,
                std::filesystem::path builtScriptsDir);
JsValue AddNativeExports(const JsFunctionArguments& args);

using NativeExportsMap =
  std::map<std::string, std::function<JsValue(const JsValue&)>>;

extern std::shared_ptr<JsEngine>* jsEngine;
extern NativeExportsMap nativeExportsMap;

inline void Register(JsValue& exports, std::shared_ptr<JsEngine>* jsEngine,
                     NativeExportsMap nativeExportsMap,
                     const std::filesystem::path& builtScriptsDir)
{
  // Register may be called multiple times, so we merge maps
  for (auto& p : nativeExportsMap)
    DevApi::nativeExportsMap.insert(p);
  DevApi::jsEngine = jsEngine;

  exports.SetProperty(
    "require",
    JsValue::Function([builtScriptsDir](const JsFunctionArguments& args) {
      return Require(args, builtScriptsDir);
    }));
  exports.SetProperty("addNativeExports", JsValue::Function(AddNativeExports));
}
}