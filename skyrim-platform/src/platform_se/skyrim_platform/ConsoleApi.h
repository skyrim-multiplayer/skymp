#pragma once

namespace ConsoleApi {

JsValue PrintConsole(const JsFunctionArguments& args);
JsValue FindConsoleCommand(const JsFunctionArguments& args);
JsValue WriteLogs(const JsFunctionArguments& args);
JsValue SetPrintConsolePrefixesEnabled(const JsFunctionArguments& args);

void Clear();
const char* GetScriptPrefix();
const char* GetExceptionPrefix();

inline void Register(JsValue& exports)
{
  exports.SetProperty("printConsole", JsValue::Function(PrintConsole));
  exports.SetProperty("findConsoleCommand",
                      JsValue::Function(FindConsoleCommand));
  exports.SetProperty("writeLogs", JsValue::Function(WriteLogs));
  exports.SetProperty("setPrintConsolePrefixesEnabled",
                      JsValue::Function(SetPrintConsolePrefixesEnabled));
}

void InitCmd(int offsetLeft, int offsetTop, int width, int height,
             bool isAlwaysOnTop);

}
