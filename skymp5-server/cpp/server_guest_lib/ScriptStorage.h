#pragma once
#include "papyrus-vm/CIString.h"
#include <cstdint>
#include <fstream>
#include <regex>
#include <set>
#include <vector>

class IScriptStorage
{
public:
  virtual ~IScriptStorage() = default;

  virtual std::vector<uint8_t> GetScriptPex(const char* scriptName) = 0;

  virtual const std::set<CIString>& ListScripts(bool forceReloadScripts) = 0;
};

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
