#pragma once
#include "HookPattern.h"

class Handler
{
public:
  Handler();

  Handler(const Napi::Value& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_,
          std::optional<HookPattern> pattern_);

  bool Matches(uint32_t selfId, const std::string& eventName);

  // Shared between threads
  const Napi::Reference<Napi::Function> enter, leave;
  const std::optional<HookPattern> pattern;
  const std::optional<double> minSelfId;
  const std::optional<double> maxSelfId;
};
