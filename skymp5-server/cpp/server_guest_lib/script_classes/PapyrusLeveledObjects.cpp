#include "PapyrusLeveledObjects.h"

#include "LeveledListUtils.h"
#include "WorldState.h"
#include "script_objects/EspmGameObject.h"

VarValue PapyrusLeveledObjects::GetNthForm(VarValue self,
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
    if (data.numEntries > static_cast<size_t>(index)) {
      auto formId = itemRecord.ToGlobalId(data.entries[index].formId);
      auto lookupRes = loader.GetBrowser().LookupById(formId);
      if (lookupRes.rec) {
        return VarValue(std::make_shared<EspmGameObject>(lookupRes));
      }
    }
  }
  return VarValue::None();
}

void PapyrusLeveledObjects::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "GetNthForm", &PapyrusLeveledObjects::GetNthForm);
}
