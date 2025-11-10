#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "SpSnippetMessage.h" // SpSnippetObjectArgument
#include "papyrus-vm/VirtualMachine.h"
#include <cstdio>

class SpSnippetFunctionGen
{
public:
  static std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>
  SerializeArguments(const std::vector<VarValue>& arguments,
                     WorldState* worldState, MpActor* optionalActor = nullptr);

  static uint32_t GetFormId(const VarValue& varValue);
};
