#pragma once
#include "SpSnippetMessage.h" // SpSnippetObjectArgument
#include "papyrus-vm/Structures.h"
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <variant>

class MpActor;
class WorldState;

enum class SpSnippetMode
{
  kNoReturnResult,
  kReturnResult,
};

class SpSnippet
{
public:
  SpSnippet(const char* cl_, const char* func_,
            const std::vector<std::optional<std::variant<
              bool, double, std::string, SpSnippetObjectArgument>>>& args_,
            uint32_t selfId_ = 0 /*zero indicates static function call*/);

  // actorExecutor will be used to detect userId to execute SpSnippet on
  Viet::Promise<VarValue> Execute(MpActor* actorExecutor, SpSnippetMode mode);

  static VarValue VarValueFromSpSnippetReturnValue(
    const std::optional<std::variant<bool, double, std::string>>& returnValue);

  static uint64_t MakeLongFormId(WorldState* worldState, uint32_t formId);

private:
  const char* const cl;
  const char* const func;
  const std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>& args;
  const uint32_t selfId;
};
