#include "PapyrusSkymp.h"

#include "MpActor.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusSkymp::SetDefaultActor(VarValue self,
                                       const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    policy->SetDefaultActor(self.GetMetaStackId(),
                            GetFormPtr<MpActor>(arguments[0]));
  }

  return VarValue::None();
}
