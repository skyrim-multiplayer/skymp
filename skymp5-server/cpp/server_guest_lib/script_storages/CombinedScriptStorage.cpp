#include "CombinedScriptStorage.h"

CombinedScriptStorage::CombinedScriptStorage(
  std::vector<std::shared_ptr<IScriptStorage>> scriptStorages)
{
  this->scriptStorages = std::move(scriptStorages);
}

std::vector<uint8_t> CombinedScriptStorage::GetScriptPex(
  const char* scriptName)
{
  for (auto& storage : scriptStorages) {
    auto result = storage->GetScriptPex(scriptName);
    if (!result.empty()) {
      return result;
    }
  }
  return {};
}

const std::set<CIString>& CombinedScriptStorage::ListScripts(
  bool forceReloadScripts)
{
  if (scripts.empty() || forceReloadScripts) {
    scripts.clear();
    for (auto& storage : scriptStorages) {
      auto& result = storage->ListScripts(forceReloadScripts);
      scripts.insert(result.begin(), result.end());
    }
  }
  return scripts;
}
