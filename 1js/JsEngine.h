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

class JsExternalObjectBase
{
public:
  virtual ~JsExternalObjectBase() = default;
};

class JsEngine
{
public:
  JsEngine()
    : pImpl(new Impl)
  {
  }

  ~JsEngine() { delete pImpl; }

  Napi::Env Env()
  {
    // TODO
    throw 1;
  }

  Napi::Value RunScript(const std::string& src, const std::string& fileName)
  {
    // TODO
    throw 1;
  }

  void ResetContext(Viet::TaskQueue<Napi::Env>& taskQueue)
  {
    // TODO
  }

  size_t GetMemoryUsage() const
  {
    // TODO
    return 0;
  }

private:
  struct Impl
  {
  };

  Impl* const pImpl;
};
