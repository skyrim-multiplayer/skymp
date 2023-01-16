#pragma once
#include <string>
#include <memory>
#include "TaskQueue.h"
#include "JsValue.h"

class JsEngine
{
  friend class JsValue;
public:
  static JsEngine CreateChakra();
  static JsEngine CreateNodeApi(void* env);

  ~JsEngine();

  JsValue RunScript(const std::string& src, const std::string& fileName);

  void ResetContext(Viet::TaskQueue& taskQueue);

  size_t GetMemoryUsage() const;

private:
  JsEngine(void* implementationDefinedEnv);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};