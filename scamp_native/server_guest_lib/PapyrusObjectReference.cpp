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
  printf("ENTER AddItem\n");

  if (arguments.size() < 2)
    return VarValue::None();
  auto item = GetRecordPtr(arguments[0]);
  auto count = static_cast<int>(arguments[1]);
  auto selfRefr = GetFormPtr<MpObjectReference>(self);

  printf("AddItem selfRefr=%p itemRec=%p count=%d\n", selfRefr, item.rec,
         count);

  if (!selfRefr || !item.rec || count <= 0)
    return VarValue::None();

  auto itemId = item.ToGlobalId(item.rec->GetId());
  selfRefr->AddItem(itemId, count);

  printf("EXIT AddItem\n");

  return VarValue::None();
}