#include "DirectoryScriptStorage.h"
#include "ScriptStorageUtils.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <spdlog/spdlog.h>

DirectoryScriptStorage::DirectoryScriptStorage(const std::string& pexDirPath_)
  : pexDir(pexDirPath_)
{
  scripts = ScriptStorageUtils::GetScriptsInDirectory(pexDir);
}

std::vector<uint8_t> DirectoryScriptStorage::GetScriptPex(
  const char* scriptName)
{
  const auto path =
    std::filesystem::path(pexDir) / (scriptName + std::string(".pex"));

  if (!std::filesystem::exists(path)) {
    spdlog::trace("DirectoryScriptStorage::GetScriptPex - Not found {} (file "
                  "doesn't exist)",
                  scriptName);
    return {};
  }

  std::ifstream f(path, std::ios::binary);
  if (!f.is_open()) {
    throw std::runtime_error(path.string() + " is failed to open");
  }
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});

  if (buffer.empty()) {
    spdlog::trace(
      "DirectoryScriptStorage::GetScriptPex - Not found {} (file is empty)",
      scriptName);
    return {};
  }

  spdlog::trace("DirectoryScriptStorage::GetScriptPex - Found {}", scriptName);
  return buffer;
}

const std::set<CIString>& DirectoryScriptStorage::ListScripts(
  bool forceReloadScripts)
{
  if (forceReloadScripts) {
    scripts = ScriptStorageUtils::GetScriptsInDirectory(pexDir);
  }
  return scripts;
}
