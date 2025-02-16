#include "IPapyrusClass.h"
#include "SpSnippetFunctionGen.h"

VarValue IPapyrusClassBase::MakeSPSnippetPromise(
  const char* script, const char* name,
  std::shared_ptr<IPapyrusCompatibilityPolicy> policy, VarValue self,
  const std::vector<VarValue>& arguments, bool method, bool returns,
  VarValue defaultResult)
{
  if (auto actor =
        policy->GetDefaultActor(script, name, self.GetMetaStackId())) {
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments, actor);
    auto promise =
      SpSnippet(script, name, s.data(),
                method ? SpSnippetFunctionGen::GetFormId(self) : 0)
        .Execute(actor,
                 (returns ? SpSnippetMode::kReturnResult
                          : SpSnippetMode::kNoReturnResult));
    if (returns)
      return VarValue(promise);
  }
  return defaultResult;
}
