#pragma once
#include "IScriptStorage.h"

class AssetsScriptStorage : public IScriptStorage
{
public:
  AssetsScriptStorage();

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override;

  const std::set<CIString>& ListScripts(bool forceReloadScripts) override;

private:
  std::set<CIString> scripts;
  CIMap<std::vector<uint8_t>> scriptPex;
};
