#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "SpSnippetFunctionGen.h"
#include "VirtualMachine.h"
#include <cstdio>

class SpSnippetFunctionGen
{
public:
  static std::string SerializeArguments(
    const std::vector<VarValue>& arguments);

  static uint32_t GetFormId(VarValue varValue);
};

#define DEFINE_STATIC_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments);             \
    if (auto actor = compatibilityPolicy->GetDefaultActor(GetName(), #name))  \
      SpSnippet(GetName(), (#name), s.data()).Execute(actor);                 \
                                                                              \
    return VarValue::None();                                                  \
  }

#define DEFINE_METHOD_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments);             \
    if (auto actor =                                                          \
          compatibilityPolicy->GetDefaultActor(GetName(), #name)) {           \
      auto promise = SpSnippet(GetName(), (#name), s.data(),                  \
                               SpSnippetFunctionGen::GetFormId(self))         \
                       .Execute(actor);                                       \
      return VarValue(promise);                                               \
    }                                                                         \
                                                                              \
    return VarValue::None();                                                  \
  }
