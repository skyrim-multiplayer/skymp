#include "ScriptStorageFactory.h"

#include <filesystem>

std::shared_ptr<IScriptStorage> ScriptStorageFactory::Create(
  nlohmann::json serverSettings)
{
  std::string dataDir = serverSettings["dataDir"];

  std::vector<std::shared_ptr<IScriptStorage>> bsaScriptStorages;
  if (serverSettings.contains("archives") &&
      serverSettings.at("archives").is_array()) {
    for (auto archive : serverSettings.at("archives")) {
      std::string archivePath = archive.get<std::string>();
      bsaScriptStorages.push_back(
        std::make_shared<BsaArchiveScriptStorage>(archivePath.data()));
    }
  }

  std::vector<std::shared_ptr<IScriptStorage>> scriptStorages;
  scriptStorages.push_back(std::make_shared<DirectoryScriptStorage>(
    (std::filesystem::path(dataDir) / "scripts").string()));
  for (auto& scriptStorage : bsaScriptStorages) {
    scriptStorages.push_back(scriptStorage);
  }
  scriptStorages.push_back(std::make_shared<AssetsScriptStorage>());
  return std::dynamic_pointer_cast<IScriptStorage>(
    std::make_shared<CombinedScriptStorage>(scriptStorages));
}
