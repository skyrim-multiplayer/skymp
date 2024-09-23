#pragma once
#include "TaskQueue.h"
#include <cstdint>
#include <cstring>
#include <fmt/ranges.h>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <napi.h>

class JsEngine
{
public:
  JsEngine()
    : pImpl(new Impl)
  {
  }

  ~JsEngine()
  {
    delete pImpl;
  }

  Napi::Env Env()
  {
    // TODO
  }

  Napi::Value RunScript(const std::string& src, const std::string& fileName)
  {
    // TODO
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

