#pragma once

class Handler
{
public:
  Handler();

  Handler(const Napi::Value& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_, std::optional<Pattern> pattern_);

  bool Matches(uint32_t selfId, const std::string& eventName);

  // PerThread structure is unique for each thread
  struct PerThread
  {
    std::shared_ptr<Napi::Reference<Napi::Object>> storage, context;
    bool matchesCondition = false;
  };
  std::unordered_map<DWORD, PerThread> perThread;

  // Shared between threads
  const Napi::Reference<Napi::Function> enter, leave;
  const std::optional<Pattern> pattern;
  const std::optional<double> minSelfId;
  const std::optional<double> maxSelfId;
};
