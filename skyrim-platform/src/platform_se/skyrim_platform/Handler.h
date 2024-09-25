#pragma once
#include "HookPattern.h"

// HandlerInfoPerThread structure is unique for each thread
struct HandlerInfoPerThread
{
  std::shared_ptr<Napi::Reference<Napi::Object>> storage, context;
  bool matchesCondition = false;
};

class Handler
{
public:
  Handler();

  Handler(const Napi::Value& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_, std::optional<HookPattern> pattern_);

  bool Matches(uint32_t selfId, const std::string& eventName);

  std::unordered_map<DWORD, HandlerInfoPerThread> perThread;

  // Shared between threads
  const Napi::Reference<Napi::Function> enter, leave;
  const std::optional<HookPattern> pattern;
  const std::optional<double> minSelfId;
  const std::optional<double> maxSelfId;
};
