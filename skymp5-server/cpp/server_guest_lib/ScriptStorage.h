#pragma once
#include "papyrus-vm/CIString.h"
#include <cstdint>
#include <fstream>
#include <memory>
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
