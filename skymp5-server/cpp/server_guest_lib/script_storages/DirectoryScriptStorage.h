#pragma once
#include "IScriptStorage.h"

class DirectoryScriptStorage : public IScriptStorage
{
public:
  DirectoryScriptStorage(const std::string& pexDir_);

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override;

  const std::set<CIString>& ListScripts(bool forceReloadScripts) override;

private:
  const std::string pexDir;
  std::set<CIString> scripts;
};
