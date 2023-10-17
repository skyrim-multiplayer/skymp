#include "ScriptStorageFactory.h"

#include <filesystem>

std::shared_ptr<IScriptStorage> ScriptStorageFactory::Create(
  nlohmann::json serverSettings)
{

  std::vector<std::shared_ptr<IScriptStorage>> scriptStorages;

  // TODO: ensure this order is correct
  AddDirectory(scriptStorages, serverSettings);
  AddBsa(scriptStorages, serverSettings);
  AddAssets(scriptStorages, serverSettings);

  return std::dynamic_pointer_cast<IScriptStorage>(
    std::make_shared<CombinedScriptStorage>(scriptStorages));
}

std::string ScriptStorageFactory::ResolveArchivePath(
  const std::string& pathFromConfigStr, const std::string& dataDir)
{
  std::filesystem::path pathFromConfig = pathFromConfigStr;

  if (pathFromConfig.is_absolute()) {
    return pathFromConfig.string();
  } else {
    return (dataDir / pathFromConfig).string();
  }
}

void ScriptStorageFactory::AddDirectory(
  std::vector<std::shared_ptr<IScriptStorage>>& storages,
  nlohmann::json& serverSettings)
{
  std::string dataDir = serverSettings["dataDir"];

  storages.push_back(std::make_shared<DirectoryScriptStorage>(
    (std::filesystem::path(dataDir) / "scripts").string()));
}

void ScriptStorageFactory::AddAssets(
  std::vector<std::shared_ptr<IScriptStorage>>& storages, nlohmann::json&)
{
  storages.push_back(std::make_shared<AssetsScriptStorage>());
}

void ScriptStorageFactory::AddBsa(
  std::vector<std::shared_ptr<IScriptStorage>>& storages,
  nlohmann::json& serverSettings)
{
  std::string dataDir = serverSettings["dataDir"];

  if (serverSettings.contains("archives") &&
      serverSettings.at("archives").is_array()) {
    for (auto& archive : serverSettings.at("archives")) {
      std::string archivePath =
        ResolveArchivePath(archive.get<std::string>(), dataDir);
      storages.push_back(
        std::make_shared<BsaArchiveScriptStorage>(archivePath.data()));
    }
  }
}
