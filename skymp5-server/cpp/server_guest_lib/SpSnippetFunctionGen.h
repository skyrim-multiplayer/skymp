#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "papyrus-vm/VirtualMachine.h"
#include <cstdio>

class SpSnippetFunctionGen
{
public:
  static std::string SerializeArguments(const std::vector<VarValue>& arguments,
                                        MpActor* actor = nullptr);

  static uint32_t GetFormId(VarValue varValue);
};
