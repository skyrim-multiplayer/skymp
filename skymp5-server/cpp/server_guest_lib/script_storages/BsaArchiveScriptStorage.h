#pragma once
#include "IScriptStorage.h"

class BsaArchiveScriptStorage : public IScriptStorage
{
public:
  BsaArchiveScriptStorage(const char* pathToBsa);

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override;

  const std::set<CIString>& ListScripts(bool forceReloadScripts) override;

private:
  std::set<CIString> scripts;
  CIMap<std::vector<uint8_t>> scriptPex;
  std::string bsaPath;
};
