#include "PapyrusObjectReference.h"

#include "EspmGameObject.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"

VarValue PapyrusObjectReference::IsDisabled(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue(false);
}

VarValue PapyrusObjectReference::GetScale(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue(1.f);
}

VarValue PapyrusObjectReference::SetScale(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::EnableNoWait(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::DisableNoWait(
  VarValue self, const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::Delete(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  return VarValue::None();
}

VarValue PapyrusObjectReference::AddItem(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 2)
    return VarValue::None();
  auto item = GetRecordPtr(arguments[0]);
  auto count = static_cast<int>(arguments[1]);
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  if (!selfRefr || !item.rec || count <= 0)
    return VarValue::None();

  auto itemId = item.ToGlobalId(item.rec->GetId());
  selfRefr->AddItem(itemId, count);

  return VarValue::None();
}

VarValue PapyrusObjectReference::RemoveItem(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 3)
    return VarValue::None();

  auto item = GetRecordPtr(arguments[0]);
  auto count = static_cast<int>(arguments[1]);
  auto selfRefr = GetFormPtr<MpObjectReference>(self);
  auto refrToAdd = GetFormPtr<MpObjectReference>(arguments[2]);

  if (!selfRefr || !item.rec)
    return VarValue::None();

  auto itemId = item.ToGlobalId(item.rec->GetId());
  auto realCount = selfRefr->GetInventory().GetItemCount(itemId);
  count = count > realCount ? realCount : count;

  selfRefr->RemoveItem(itemId, count, refrToAdd);

  return VarValue::None();
}

VarValue PapyrusObjectReference::GetItemCount(
  VarValue self, const std::vector<VarValue>& arguments)
{
  if (arguments.size() >= 1) {
    auto selfRefr = GetFormPtr<MpObjectReference>(self);
    auto& form = GetRecordPtr(arguments[0]);

    const uint32_t formId = form.ToGlobalId(form.rec->GetId());

    if (selfRefr) {
      return VarValue(
        static_cast<int>(selfRefr->GetInventory().GetItemCount(formId)));
    }
  }
  return VarValue(0);
}