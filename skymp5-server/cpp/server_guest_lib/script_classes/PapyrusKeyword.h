#pragma once
#include "IPapyrusClass.h"

class PapyrusKeyword final : public IPapyrusClass<PapyrusKeyword>
{
public:
  const char* GetName() override { return "Keyword"; }

  VarValue GetKeyword(VarValue self, const std::vector<VarValue>& arguments);

  void Register(VirtualMachine& vm,
                std::shared_ptr<IPapyrusCompatibilityPolicy> policy) override;

  std::vector<const std::vector<const espm::RecordHeader*>*> keywords;
};
