#include "PapyrusPotion.h"
#include "script_objects/EspmGameObject.h"

VarValue PapyrusPotion::IsFood(VarValue self,
                               const std::vector<VarValue>& arguments)
{
  const auto& item = GetRecordPtr(self);

  if (!item.rec) {
    return VarValue(false);
  }

  auto alch = espm::Convert<espm::ALCH>(item.rec);

  if (!alch) {
    return VarValue(false);
  }

  espm::CompressedFieldsCache cache;
  espm::ALCH::Data data = alch->GetData(cache);
  return VarValue(data.isFood);
}
