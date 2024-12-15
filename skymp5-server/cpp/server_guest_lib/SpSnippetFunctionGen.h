#pragma once
#include "MpActor.h"
#include "SpSnippet.h"
#include "SpSnippetFunctionGen.h"
#include "SpSnippetMessage.h" // SpSnippetObjectArgument
#include "papyrus-vm/VirtualMachine.h"
#include <cstdio>

class SpSnippetFunctionGen
{
public:
  static std::vector<std::optional<
    std::variant<bool, double, std::string, SpSnippetObjectArgument>>>
  SerializeArguments(const std::vector<VarValue>& arguments,
                     MpActor* actor = nullptr);

  static uint32_t GetFormId(const VarValue& varValue);
};

// TODO: unhardcode mode
#define DEFINE_STATIC_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    if (auto actor = compatibilityPolicy->GetDefaultActor(                    \
          GetName(), #name, self.GetMetaStackId())) {                         \
      auto s = SpSnippetFunctionGen::SerializeArguments(arguments, actor);    \
      SpSnippet(GetName(), (#name), s)                                        \
        .Execute(actor, SpSnippetMode::kNoReturnResult);                      \
    }                                                                         \
    return VarValue::None();                                                  \
  }

// TODO: unhardcode mode
#define DEFINE_METHOD_SPSNIPPET(name)                                         \
  VarValue name(VarValue self, const std::vector<VarValue>& arguments)        \
  {                                                                           \
    if (auto actor = compatibilityPolicy->GetDefaultActor(                    \
          GetName(), #name, self.GetMetaStackId())) {                         \
      auto s = SpSnippetFunctionGen::SerializeArguments(arguments, actor);    \
      auto promise = SpSnippet(GetName(), (#name), s,                         \
                               SpSnippetFunctionGen::GetFormId(self))         \
                       .Execute(actor, SpSnippetMode::kReturnResult);         \
      return VarValue(promise);                                               \
    }                                                                         \
                                                                              \
    return VarValue::None();                                                  \
  }
