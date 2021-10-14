#include "PapyrusFormList.h"

#include "EspmGameObject.h"

VarValue PapyrusFormList::GetSize(VarValue self,
                                  const std::vector<VarValue>& arguments)
{
  espm::CompressedFieldsCache dummyCache;

  auto res = GetRecordPtr(self);
  if (auto formlist = espm::Convert<espm::FLST>(res.rec)) {
    int size = static_cast<int>(formlist->GetData(dummyCache).formIds.size());
    return VarValue(size);
  }
  return VarValue(0);
}

VarValue PapyrusFormList::GetAt(VarValue self,
                                const std::vector<VarValue>& arguments)
{
  espm::CompressedFieldsCache dummyCache;

  if (arguments.size() >= 1) {
    int idx = static_cast<int>(arguments[0]);
    auto res = GetRecordPtr(self);
    if (auto formlist = espm::Convert<espm::FLST>(res.rec)) {
      auto formIds = formlist->GetData(dummyCache).formIds;
      if (idx >= 0 && static_cast<int>(formIds.size()) > idx) {
        auto formId = res.ToGlobalId(formIds[idx]);
        auto record = res.parent->LookupById(formId);
        return VarValue(std::make_shared<EspmGameObject>(record));
      }
    }
  }
  return VarValue::None();
}
