#include "InGameConsolePrinter.h"
#include "ConsoleApi.h"
#include "NullPointerException.h"

void InGameConsolePrinter::Print(const Napi::CallbackInfo& info)
{
  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    throw NullPointerException("console");

  std::string s;

  for (size_t i = 0; i < info.Length(); ++i) {
    Napi::Value str = info[i];

    if (info[i].IsObject() && !info[i].IsExternal()) {
      Napi::Object global = env.Global();
      Napi::Object json = global.Get("JSON").As<Napi::Object>();
      Napi::Function stringify = json.Get("stringify").As<Napi::Function>();
      str = stringify.Call(json, { info[i] });
    }

    s += str.ToString().Utf8Value() + " ";
  }

  int maxSize = 128;
  if (s.size() > maxSize) {
    s.resize(maxSize);
    s += "...";
  }

  const char* prefix = ConsoleApi::GetScriptPrefix();
  console->Print("%s%s", prefix, s.data());
}

void InGameConsolePrinter::PrintRaw(const char* str)
{
  auto console = RE::ConsoleLog::GetSingleton();
  if (!console)
    throw NullPointerException("console");

  console->Print(str);
}
