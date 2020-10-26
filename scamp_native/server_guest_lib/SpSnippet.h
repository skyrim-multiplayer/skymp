#pragma once
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

class MpActor;

class SpSnippet
{
public:
  SpSnippet(const char* cl_, const char* func_, const char* args_,
            uint32_t selfId_ = 0);
  void Send(MpActor* actor, std::function<void(nlohmann::json)> cb = nullptr);

private:
  const char *const cl, *const func, *const args;
  const uint32_t selfId;
};