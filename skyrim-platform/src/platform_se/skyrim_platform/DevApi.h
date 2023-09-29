#pragma once

namespace DevApi {
JsValue Require(const JsFunctionArguments& args,
                const std::vector<const char*>& pluginLoadDirectories);
JsValue AddNativeExports(const JsFunctionArguments& args);

JsValue GetPluginSourceCode(const JsFunctionArguments& args);

JsValue WritePlugin(const JsFunctionArguments& args);

JsValue GetPlatformVersion(const JsFunctionArguments& args);

JsValue GetJsMemoryUsage(const JsFunctionArguments& args);

JsValue BlockPapyrusEvents(const JsFunctionArguments& args);

void DisableCtrlPrtScnHotkey();

using NativeExportsMap =
  std::map<std::string, std::function<JsValue(const JsValue&)>>;

extern std::shared_ptr<JsEngine> jsEngine;
extern NativeExportsMap nativeExportsMap;

inline void Register(JsValue& exports, std::shared_ptr<JsEngine> jsEngine,
                     NativeExportsMap nativeExportsMap,
                     const std::vector<const char*>& builtScriptsDir)
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
  exports.SetProperty("getPluginSourceCode",
                      JsValue::Function(GetPluginSourceCode));
  exports.SetProperty("writePlugin", JsValue::Function(WritePlugin));
  exports.SetProperty("getPlatformVersion",
                      JsValue::Function(GetPlatformVersion));
  exports.SetProperty("getJsMemoryUsage", JsValue::Function(GetJsMemoryUsage));
  exports.SetProperty("disableCtrlPrtScnHotkey",
                      JsValue::Function([](const JsFunctionArguments& args) {
                        DisableCtrlPrtScnHotkey();
                        return JsValue::Undefined();
                      }));
  exports.SetProperty("blockPapyrusEvents",
                      JsValue::Function(BlockPapyrusEvents));
}
}
