#include "PapyrusLeveledItem.h"

#include "LeveledListUtils.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"

VarValue PapyrusLeveledItem::GetNthForm(VarValue self,
                                        const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1) {
    throw std::runtime_error(
      "LeveledItem.GetNthForm requires at least 1 argument");
  }

  auto itemRecord = GetRecordPtr(self);
  if (!itemRecord.rec) {
    throw std::runtime_error("Self not found");
  }
  auto& loader = compatibilityPolicy->GetWorldState()->GetEspm();
  auto leveledItem = espm::Convert<espm::LVLI>(itemRecord.rec);
  if (leveledItem) {
    auto vec =
      LeveledListUtils::EvaluateList(loader.GetBrowser(), itemRecord, 1);
    int index = static_cast<int>(arguments[0]);
    if (vec.size() > index) {
      auto formId = itemRecord.ToGlobalId(vec[index].formId);
      auto record = itemRecord.parent->LookupById(formId);
      return VarValue(std::make_shared<EspmGameObject>(record));
    }
  }
  return VarValue::None();
}

void PapyrusLeveledItem::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "GetNthForm", &PapyrusLeveledItem::GetNthForm);
}
