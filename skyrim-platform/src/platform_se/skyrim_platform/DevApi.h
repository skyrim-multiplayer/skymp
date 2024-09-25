#pragma once

#include "NapiHelper.h"

namespace DevApi {
Napi::Value Require(
  const Napi::CallbackInfo& info,
  const std::vector<std::filesystem::path>& pluginLoadDirectories);
Napi::Value AddNativeExports(const Napi::CallbackInfo& info);

Napi::Value GetPluginSourceCode(const Napi::CallbackInfo& info);

Napi::Value WritePlugin(const Napi::CallbackInfo& info);

Napi::Value GetPlatformVersion(const Napi::CallbackInfo& info);

Napi::Value GetJsMemoryUsage(const Napi::CallbackInfo& info);

Napi::Value BlockPapyrusEvents(const Napi::CallbackInfo& info);

void DisableCtrlPrtScnHotkey();

using NativeExportsMap =
  std::map<std::string, std::function<Napi::Object(const Napi::Value&)>>;

extern std::shared_ptr<JsEngine> jsEngine;
extern NativeExportsMap nativeExportsMap;

inline void Register(Napi::Env env, Napi::Object& exports,
                     std::shared_ptr<JsEngine> jsEngine,
                     NativeExportsMap nativeExportsMap,
                     const std::vector<std::filesystem::path>& builtScriptsDir)
{
  // Register may be called multiple times, so we merge maps
  for (auto& p : nativeExportsMap) {
    DevApi::nativeExportsMap.insert(p);
  }
  DevApi::jsEngine = jsEngine;

  exports.Set(
    "require",
    Napi::Function::New(
      env,
      NapiHelper::WrapCppExceptions(
        [builtScriptsDir](const Napi::CallbackInfo& info) -> Napi::Value {
          return Require(info, builtScriptsDir);
        })));
  exports.Set(
    "addNativeExports",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(AddNativeExports)));
  exports.Set("getPluginSourceCode",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetPluginSourceCode)));
  exports.Set(
    "writePlugin",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(WritePlugin)));
  exports.Set("getPlatformVersion",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetPlatformVersion)));
  exports.Set(
    "getJsMemoryUsage",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetJsMemoryUsage)));
  exports.Set(
    "disableCtrlPrtScnHotkey",
    Napi::Function::New(env,
                        NapiHelper::WrapCppExceptions(
                          [](const Napi::CallbackInfo& info) -> Napi::Value {
                            DisableCtrlPrtScnHotkey();
                            return info.Env().Undefined();
                          })));
  exports.Set("blockPapyrusEvents",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(BlockPapyrusEvents)));
}
}
