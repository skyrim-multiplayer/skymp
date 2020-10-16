#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

class IScriptStorage
{
public:
  virtual ~IScriptStorage() = default;

  virtual std::vector<uint8_t> GetScriptPex(const char* scriptName) = 0;
};

class DirectoryScriptStorage : public IScriptStorage
{
public:
  DirectoryScriptStorage(std::filesystem::path pexDir_)
    : pexDir(pexDir_)
  {
  }

  std::vector<uint8_t> GetScriptPex(const char* scriptName) override
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

  const std::filesystem::path pexDir;
};