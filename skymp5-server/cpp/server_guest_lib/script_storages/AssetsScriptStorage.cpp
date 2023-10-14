#include "AssetsScriptStorage.h"

#include <cmrc/cmrc.hpp>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <sstream>

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
