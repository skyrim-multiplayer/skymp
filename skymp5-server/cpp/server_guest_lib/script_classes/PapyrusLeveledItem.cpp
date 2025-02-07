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
    auto data = leveledItem->GetData(
      compatibilityPolicy->GetWorldState()->GetEspmCache());
    int index = static_cast<int>(arguments[0]);
    if (static_cast<int>(data.numEntries) > index) {
      auto formId = itemRecord.ToGlobalId(data.entries[index].formId);
      auto record = loader->GetBrowser().LookupById(formId);
      if (record) {
        return VarValue(std::make_shared<EspmGameObject>(record));
      }
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
