#pragma once
#include "AssetsScriptStorage.h"
#include "BsaArchiveScriptStorage.h"
#include "CombinedScriptStorage.h"
#include "DirectoryScriptStorage.h"

#include <nlohmann/json.hpp>
#include <string>

class ScriptStorageFactory
{
public:
  static std::shared_ptr<IScriptStorage> Create(nlohmann::json serverSettings);

private:
  static std::string ResolveArchivePath(const std::string& pathFromConfigStr,
                                        const std::string& dataDir);

  static void AddDirectory(
    std::vector<std::shared_ptr<IScriptStorage>>& storages,
    nlohmann::json &serverSettings);
  static void AddAssets(std::vector<std::shared_ptr<IScriptStorage>>& storages,
                        nlohmann::json &serverSettings);
  static void AddBsa(std::vector<std::shared_ptr<IScriptStorage>>& storages,
                     nlohmann::json &serverSettings);
};
