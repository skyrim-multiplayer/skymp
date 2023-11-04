#pragma once
#include "IScriptStorage.h"

#include <memory>

class CombinedScriptStorage : public IScriptStorage
{
public:
  // Load order matters. But unlike mods, scriptStorages.front() will be
  // checked first
  CombinedScriptStorage(
    std::vector<std::shared_ptr<IScriptStorage>> scriptStorages);

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override;

  const std::set<CIString>& ListScripts(bool forceReloadScripts) override;

private:
  std::vector<std::shared_ptr<IScriptStorage>> scriptStorages;
  std::set<CIString> scripts;
};
