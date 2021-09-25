#include "PapyrusSkymp.h"

#include "HeuristicPolicy.h"
#include "MpFormGameObject.h"

VarValue PapyrusSkymp::SetDefaultActor(VarValue self,
                                       const std::vector<VarValue>& arguments)
{
  auto heuristicPolicy = std::dynamic_pointer_cast<HeuristicPolicy>(policy);
  if (!heuristicPolicy)
    throw std::runtime_error(
      "Current compatibility policy doesn't support SetDefaultActor");

  if (arguments.size() >= 1)
    heuristicPolicy->SetDefaultActor(self.GetMetaStackId(),
                                     GetFormPtr<MpActor>(arguments[0]));

  return VarValue::None();
}
