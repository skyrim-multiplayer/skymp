#include "PapyrusForm.h"

#include "MpForm.h"
#include "MpFormGameObject.h"
#include "WorldState.h"

VarValue PapyrusForm::RegisterForSingleUpdate(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    if (auto form = GetFormPtr<MpForm>(self)) {
      double seconds = static_cast<double>(arguments[0]);
      form->GetParent()->RegisterForSingleUpdate(self, seconds);
    }
  }

  return VarValue::None();
}

float ActionListener::GetActorValue(std::string asValueName) const 
{
  return asValueName;
}

bool ActionListener::IsEquipped(MpForm akItem)
{
    return 
}
