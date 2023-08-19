#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "SpSnippetFunctionGen.h"
#include "papyrus-vm/VirtualMachine.h"
#include <cstdio>

class SpSnippetFunctionGen
{
public:
  static std::string SerializeArguments(const std::vector<VarValue>& arguments,
                                        MpActor* actor = nullptr);

  static uint32_t GetFormId(VarValue varValue);
};

#define DEFINE_STATIC_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    if (auto actor = compatibilityPolicy->GetDefaultActor(                    \
          GetName(), #name, self.GetMetaStackId())) {                         \
      auto s = SpSnippetFunctionGen::SerializeArguments(arguments, actor);    \
      SpSnippet(GetName(), (#name), s.data()).Execute(actor);                 \
    }                                                                         \
    return VarValue::None();                                                  \
  }

#define DEFINE_METHOD_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    if (auto actor = compatibilityPolicy->GetDefaultActor(                    \
          GetName(), #name, self.GetMetaStackId())) {                         \
      auto s = SpSnippetFunctionGen::SerializeArguments(arguments, actor);    \
      auto promise = SpSnippet(GetName(), (#name), s.data(),                  \
                               SpSnippetFunctionGen::GetFormId(self))         \
                       .Execute(actor);                                       \
      return VarValue(promise);                                               \
    }                                                                         \
                                                                              \
    return VarValue::None();                                                  \
  }
