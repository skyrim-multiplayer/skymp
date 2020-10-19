#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "SpSnippetFunctionGen.h"
#include "VirtualMachine.h"
#include <cstdio>

#define DEFINE_STATIC_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments);             \
    if (auto actor = compatibilityPolicy->GetDefaultActor())                  \
      SpSnippet(GetName(), (#name), s.data()).Send(actor);                    \
                                                                              \
    return VarValue::None();                                                  \
  }

class SpSnippetFunctionGen
{
public:
  static std::string SerializeArguments(
    const std::vector<VarValue>& arguments);
};