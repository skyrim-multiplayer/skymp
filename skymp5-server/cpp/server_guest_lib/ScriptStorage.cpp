#include "ScriptStorage.h"
#include <cstring>
#include <filesystem>

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

  if (!std::filesystem::exists(path))
    return {};

  std::ifstream f(path, std::ios::binary);
  if (!f.is_open())
    throw std::runtime_error(path.string() + " is failed to open");
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
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
