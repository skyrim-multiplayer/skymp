#include "PapyrusForm.h"

#include "MpForm.h"
#include "MpFormGameObject.h"
#include "WorldState.h"

VarValue PapyrusForm::RegisterForSingleUpdate(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    if (auto form = GetFormPtr<MpForm>(self)) {
      float seconds = static_cast<float>(arguments[0]);
      form->GetParent()->RegisterForSingleUpdate(self, seconds);
    }
  }

  return VarValue::None();
}