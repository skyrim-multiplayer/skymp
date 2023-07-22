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
};

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject);
