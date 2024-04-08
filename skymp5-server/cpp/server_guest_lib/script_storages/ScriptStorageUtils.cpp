#include "ScriptStorageUtils.h"

#include <filesystem>
#include <regex>

std::string ScriptStorageUtils::GetFileName(const std::string& path)
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

std::string ScriptStorageUtils::RemoveExtension(std::string s)
{
  const std::regex e(".*\\.pex");
  if (std::regex_match(s, e)) {
    s = { s.begin(), s.end() - strlen(".pex") };
    return s;
  }
  return "";
}

std::set<CIString> ScriptStorageUtils::GetScriptsInDirectory(
  const std::string& pexDir)
{
  std::set<CIString> scripts;

  for (auto& p : std::filesystem::directory_iterator(pexDir)) {
    if (p.is_directory()) {
      continue;
    }

    std::string s = GetFileName(p.path().string());
    if (auto fileNameWe = RemoveExtension(s); !fileNameWe.empty()) {
      scripts.insert({ fileNameWe.begin(), fileNameWe.end() });
    }
  }

  return scripts;
}
