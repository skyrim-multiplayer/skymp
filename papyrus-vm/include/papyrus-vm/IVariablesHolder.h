#pragma once

class VarValue;
class PexScript;

class IVariablesHolder
{
public:
  virtual ~IVariablesHolder() = default;

  // Must guarantee that no exception would be thrown for '::State' variable
  virtual VarValue* GetVariableByName(const char* name,
                                      const PexScript& pex) = 0;
};
