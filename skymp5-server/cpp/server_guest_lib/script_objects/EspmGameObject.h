#pragma once
#include "libespm/Combiner.h"
#include "papyrus-vm/VirtualMachine.h"

class EspmGameObject : public IGameObject
{
public:
  explicit EspmGameObject(const espm::LookupResult& record_);

  const char* GetParentNativeScript() override;
  bool EqualsByValue(const IGameObject& obj) const override;
  const char* GetStringID() override;

  const espm::LookupResult record;

protected:
  const std::vector<std::shared_ptr<ActivePexInstance>>&
  ListActivePexInstances() const override;

  void AddScript(std::shared_ptr<ActivePexInstance> sctipt) noexcept override;
};

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject);
