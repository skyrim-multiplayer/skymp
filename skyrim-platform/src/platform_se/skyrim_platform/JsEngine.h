#pragma once
#include "TaskQueue.h"
#include <cstdint>
#include <cstring>
#include <fmt/ranges.h>
#include <memory>
#include <napi.h>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fmt/format.h>
#include <iostream>

#include <Windows.h>

#include "NapiHelper.h"

class JsEngine
{
  friend Napi::Value CallPreparedFunction(const Napi::CallbackInfo& info);
public:
  static JsEngine* GetSingleton();
  
  ~JsEngine();

  void AcquireEnvAndCall(const std::function<void(Napi::Env)>& f);
  Napi::Value RunScript(Napi::Env env, const std::string& src, const std::string&);
  void ResetContext(Viet::TaskQueue<Napi::Env>&);
  size_t GetMemoryUsage() const;

private:
  JsEngine();

  std::string GetError();

  struct Impl;
  Impl* const pImpl;
};
