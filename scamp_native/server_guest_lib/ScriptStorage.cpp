#include "ScriptStorage.h"

namespace {
std::string GetFileName(const std::filesystem::path& p)
{
  std::string s = p.string();
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
}

DirectoryScriptStorage::DirectoryScriptStorage(
  const std::filesystem::path& pexDir_)
  : pexDir(pexDir_)
{
  for (auto& p : std::filesystem::directory_iterator(pexDir)) {
    if (p.is_directory())
      continue;

    std::string s = GetFileName(p.path());
    if (auto fileNameWe = RemoveExtension(s); !fileNameWe.empty())
      scripts.insert(fileNameWe);
  }
}

std::vector<uint8_t> DirectoryScriptStorage::GetScriptPex(
  const char* scriptName)
{
  const auto path = pexDir / (scriptName + std::string(".pex"));

  if (!std::filesystem::exists(path))
    return {};

  std::ifstream f(path, std::ios::binary);
  if (!f.is_open())
    throw std::runtime_error(path.string() + " is failed to open");
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(f), {});
  return buffer;
}

const std::set<std::string>& DirectoryScriptStorage::ListScripts()
{
  return scripts;
}
