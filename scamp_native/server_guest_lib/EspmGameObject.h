#pragma once
#include "Combiner.h"
#include "VirtualMachine.h"

class EspmGameObject : public IGameObject
{
public:
  EspmGameObject(const espm::LookupResult& record_);

  const char* GetParentNativeScript() override;
  bool EqualsByValue(const IGameObject& obj) const override;

  const espm::LookupResult record;
};

const espm::LookupResult& GetRecordPtr(const VarValue& papyrusObject);