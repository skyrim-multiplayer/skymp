#include "ScriptStorage.h"
#include <cmrc/cmrc.hpp>
#include <cstring>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sstream>

namespace {
std::string GetFileName(const std::string& path)
{
  std::string s = path;
  while (s.find('/') != s.npos || s.find('\\') != s.npos) {
    while (s.find('/') != s.npos)
      s = { s.begin() + s.find('/') + 1, s.end() };
    while (s.find('\\') != s.npos)
      s = { s.begin() + s.find('\\') + 1, s.end() };
  }
  return s;
}

std::string RemoveExtension(std::string s)
{
  const std::regex e(".*\\.pex");
  if (std::regex_match(s, e)) {
    s = { s.begin(), s.end() - strlen(".pex") };
    return s;
  }
  return "";
}

std::set<CIString> GetScriptsInDirectory(std::string pexDir)
{
  std::set<CIString> scripts;

  for (auto& p : std::filesystem::directory_iterator(pexDir)) {
    if (p.is_directory())
      continue;

    std::string s = GetFileName(p.path().string());
    if (auto fileNameWe = RemoveExtension(s); !fileNameWe.empty())
      scripts.insert({ fileNameWe.begin(), fileNameWe.end() });
  }

  return scripts;
}
}

DirectoryScriptStorage::DirectoryScriptStorage(const std::string& pexDirPath_)
  : pexDir(pexDirPath_)
{
  scripts = GetScriptsInDirectory(pexDir);
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
    scripts = GetScriptsInDirectory(pexDir);
  }
  return scripts;
}

CMRC_DECLARE(server_standard_scripts);

AssetsScriptStorage::AssetsScriptStorage()
{
  try {
    auto fileSystem = cmrc::server_standard_scripts::get_filesystem();
    for (auto&& entry : fileSystem.iterate_directory("standard_scripts")) {
      cmrc::file file =
        fileSystem.open("standard_scripts/" + entry.filename());
      const uint8_t* begin = reinterpret_cast<const uint8_t*>(file.begin());
      const uint8_t* end = begin + file.size();
      std::vector<uint8_t> pex(begin, end);
      auto nameWithoutExtension =
        std::filesystem::path(entry.filename()).stem().string();
      scripts.insert(
        { nameWithoutExtension.begin(), nameWithoutExtension.end() });
      scriptPex[{ nameWithoutExtension.begin(), nameWithoutExtension.end() }] =
        pex;
    }
  } catch (std::exception& e) {
    auto dir =
      cmrc::server_standard_scripts::get_filesystem().iterate_directory("");
    std::stringstream ss;
    ss << e.what() << std::endl << std::endl;
    ss << "Root directory contents is: " << std::endl;
    for (auto&& entry : dir) {
      ss << entry.filename() << std::endl;
    }
    throw std::runtime_error(ss.str());
  }
}

std::vector<uint8_t> AssetsScriptStorage::GetScriptPex(const char* scriptName)
{
  auto it = scriptPex.find(scriptName);
  if (it == scriptPex.end()) {
    spdlog::trace("AssetsScriptStorage::GetScriptPex - Not found {}",
                  scriptName);
    return {};
  }
  spdlog::trace("AssetsScriptStorage::GetScriptPex - Found {}", scriptName);
  return it->second;
}

const std::set<CIString>& AssetsScriptStorage::ListScripts(bool)
{
  return scripts;
}

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
  scripts.clear();
  for (auto& storage : scriptStorages) {
    auto& result = storage->ListScripts(forceReloadScripts);
    scripts.insert(result.begin(), result.end());
  }
  return scripts;
}
