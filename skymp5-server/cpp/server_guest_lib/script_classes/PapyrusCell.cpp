#include "PapyrusCell.h"

VarValue PapyrusCell::IsAttached(VarValue self,
                                 const std::vector<VarValue>& arguments)
{
  return VarValue(true); // stub
}

VarValue PapyrusCell::IsInterior(VarValue self,
                                 const std::vector<VarValue>& arguments)
{
  if (auto lookupRes = GetRecordPtr(self); lookupRes.rec) {
    espm::CompressedFieldsCache cache;

    auto cell = espm::Convert<espm::CELL>(lookupRes.rec);
    if (cell) {
      bool isInterior = cell->GetData(cache).flags & espm::CELL::Interior;
      return VarValue(isInterior);
    } else {
      spdlog::error(
        "PapyrusCell::IsInterior: failed to convert record to CELL");
    }
  } else {
    spdlog::error("PapyrusCell::IsInterior: record not found");
  }
  return VarValue(false);
}

void PapyrusCell::Register(VirtualMachine& vm,
                           std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "IsAttached", &PapyrusCell::IsAttached);
  AddMethod(vm, "IsInterior", &PapyrusCell::IsInterior);
}
