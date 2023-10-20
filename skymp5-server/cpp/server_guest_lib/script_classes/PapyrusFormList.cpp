#include "PapyrusFormList.h"

#include "script_objects/EspmGameObject.h"

VarValue PapyrusFormList::GetSize(VarValue self,
                                  const std::vector<VarValue>& arguments)
{
  espm::CompressedFieldsCache dummyCache;

  const auto& res = GetRecordPtr(self);
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
    const auto& res = GetRecordPtr(self);
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

VarValue PapyrusFormList::Find(VarValue self,
                               const std::vector<VarValue>& arguments) const
{
  espm::CompressedFieldsCache dummyCache;

  if (arguments.size() >= 1) {
    const auto& res = GetRecordPtr(self);
    if (auto formlist = espm::Convert<espm::FLST>(res.rec)) {
      const auto& arg = GetRecordPtr(arguments[0]);
      if (arg.rec != nullptr) {
        auto formId = arg.ToGlobalId(arg.rec->GetId());
        auto data = formlist->GetData(dummyCache).formIds;
        for (int i = 0; i < data.size(); i++) {
          if (data[i] == formId) {
            return VarValue(i);
          }
        }
      }
    }
  }

  return VarValue::None();
}
