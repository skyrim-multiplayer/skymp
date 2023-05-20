#include "PapyrusForm.h"

#include "MpActor.h"
#include "MpFormGameObject.h"
#include "WorldState.h"
#include "EspmGameObject.h"

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

VarValue PapyrusForm::GetType(VarValue self, const std::vector<VarValue>&)
{
  const auto& selfRec = GetRecordPtr(self);
  if (selfRec.rec) {
    return VarValue(static_cast<int32_t>(selfRec.rec->GetType().ToUint32()));
  }

  if (auto form = GetFormPtr<MpForm>(self)) {
    if (dynamic_cast<MpActor *>(form)) {
      constexpr auto kCharacter = 62;
      return VarValue(static_cast<int32_t>(kCharacter));
    }
    if (dynamic_cast<MpObjectReference *>(form)) {
      constexpr auto kReference = 61;
      return VarValue(static_cast<int32_t>(kReference));
    }
  }

  return VarValue::None();
}
