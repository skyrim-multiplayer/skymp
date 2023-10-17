#include "BsaArchiveScriptStorage.h"

#include <bsa/tes4.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <stdexcept>

BsaArchiveScriptStorage::BsaArchiveScriptStorage(const char* bsaPath_)
{
  this->bsaPath = bsaPath_;
  if (!std::filesystem::exists(bsaPath_)) {
    throw std::runtime_error(
      fmt::format("BSA Archive '{}' doesn't exist", bsaPath_));
  }
}

std::vector<uint8_t> BsaArchiveScriptStorage::GetScriptPex(
  const char* scriptName)
{
  auto it = scriptPex.find(scriptName);
  if (it == scriptPex.end()) {
    spdlog::trace("BsaArchiveScriptStorage::GetScriptPex - Not found {}",
                  scriptName);
    return {};
  }
  spdlog::trace("BsaArchiveScriptStorage::GetScriptPex - Found {}",
                scriptName);
  return it->second;
}

const std::set<CIString>& BsaArchiveScriptStorage::ListScripts(
  bool forceReloadScripts)
{
  if (scripts.empty() || forceReloadScripts) {
    scripts.clear();
    bsa::tes4::archive bsa;
    bsa.read(bsaPath);
    auto bsaScripts = *bsa["scripts"];
    for (auto it = bsaScripts.begin(); it != bsaScripts.end(); it++) {
      auto fileName = it->first.name();

      const std::byte* data = it->second.data();
      size_t size = it->second.size();
      auto pex =
        std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(data),
                             reinterpret_cast<const uint8_t*>(data) + size);

      auto nameWithoutExtension =
        std::filesystem::path(fileName).stem().string();
      scripts.insert(
        { nameWithoutExtension.begin(), nameWithoutExtension.end() });
      scriptPex[{ nameWithoutExtension.begin(), nameWithoutExtension.end() }] =
        pex;
    }
  }
  return scripts;
}
