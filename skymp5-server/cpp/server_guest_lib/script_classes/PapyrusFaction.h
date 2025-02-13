#pragma once
#include "IPapyrusClass.h"

namespace espm {
class RecordHeader;
}

class PapyrusFaction final : public IPapyrusClass<PapyrusFaction>
{
public:
  const char* GetName() override { return "Faction"; }

  VarValue GetReaction(VarValue self, const std::vector<VarValue>& arguments);
  // SetReaction ignored, because no way to edit factions forever?

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::vector<const std::vector<const espm::RecordHeader*>*> factions;
};
