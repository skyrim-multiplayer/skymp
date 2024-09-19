#pragma once

#include "NapiHelper.h"

namespace ConsoleApi {

Napi::Value PrintConsole(const Napi::CallbackInfo &info);
Napi::Value FindConsoleCommand(const Napi::CallbackInfo &info);
Napi::Value WriteLogs(const Napi::CallbackInfo &info);
Napi::Value SetPrintConsolePrefixesEnabled(const Napi::CallbackInfo &info);

void Clear();
const char* GetScriptPrefix();
const char* GetExceptionPrefix();

inline void Register(Napi::Env env, Napi::Value& exports)
{
  exports.SetProperty("printConsole", Napi::Function::New(env, NapiHelper::WrapCppExceptions(PrintConsole)));
  exports.SetProperty("findConsoleCommand",
                      Napi::Function::New(env, NapiHelper::WrapCppExceptions(FindConsoleCommand)));
  exports.SetProperty("writeLogs", Napi::Function::New(env, WriteLogs));
  exports.SetProperty("setPrintConsolePrefixesEnabled",
                      Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetPrintConsolePrefixesEnabled)));
}

void InitCmd(int offsetLeft, int offsetTop, int width, int height,
             bool isAlwaysOnTop);

}
