#pragma once
#include "CIString.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <vector>

class IScriptStorage
{
public:
  virtual ~IScriptStorage() = default;

  virtual std::vector<uint8_t> GetScriptPex(const char* scriptName) = 0;

  virtual const std::set<CIString>& ListScripts() = 0;
};

class DirectoryScriptStorage : public IScriptStorage
{
public:
  DirectoryScriptStorage(const std::filesystem::path& pexDir_);

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override;

  const std::set<CIString>& ListScripts() override;

  const std::filesystem::path pexDir;
  std::set<CIString> scripts;
};