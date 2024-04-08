#pragma once
#include "papyrus-vm/Structures.h"
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

class MpActor;

enum class SpSnippetMode
{
  kNoReturnResult,
  kReturnResult,
};

class SpSnippet
{
public:
  SpSnippet(const char* cl_, const char* func_, const char* args_,
            uint32_t selfId_ = 0);

  Viet::Promise<VarValue> Execute(MpActor* actor, SpSnippetMode mode);

private:
  const char *const cl, *const func, *const args;
  const uint32_t selfId;
};
