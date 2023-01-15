#pragma once
#include <string>
#include "TaskQueue.h"
#include "JsValue.h"

class JsEngine
{
  friend class JsValue;
public:
  JsEngine();
  ~JsEngine();

  JsValue RunScript(const std::string& src, const std::string& fileName);

  void ResetContext(Viet::TaskQueue& taskQueue);

  size_t GetMemoryUsage() const;

private:
  struct Impl;
  Impl* const pImpl;
};