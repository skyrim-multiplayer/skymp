#define CATCH_CONFIG_RUNNER
#include <Loader.h>
#include <catch2/catch.hpp>
#include <iostream>

namespace {
inline void OnProgress(const std::string& fileName, float readDuration,
                       float parseDuration, uintmax_t fileSize)
{
  std::cout << "[ESPM] " << fileName << " read in " << readDuration
            << "s, parsed in " << parseDuration << "s, size is "
            << (fileSize / 1024 / 1024) << "Mb" << std::endl;
}

inline bool IsCmakeOptionSpecified(const std::string& optionValue)
{
  return !optionValue.empty() && optionValue != "OFF";
}

inline const char* GetDataDir()
{
  return IsCmakeOptionSpecified(UNIT_DATA_DIR) ? UNIT_DATA_DIR
                                               : SKYRIM_DIR "/Data";
}

inline espm::Loader CreateEspmLoader()
{
  try {
    std::vector<std::filesystem::path> files = { "Skyrim.esm", "Update.esm",
                                                 "Dawnguard.esm",
                                                 "HearthFires.esm",
                                                 "Dragonborn.esm" };

    std::filesystem::path dataDir = std::filesystem::u8path(GetDataDir());

    if (!std::filesystem::exists(dataDir / "Skyrim.esm")) {
      files.clear();
      dataDir = std::filesystem::current_path();
    }

    return espm::Loader(dataDir, files, OnProgress);
  } catch (std::exception& e) {
    std::cout << "Exception in CreateEspmLoader:" << std::endl;
    std::cout << e.what() << std::endl;
    std::exit(1);
  }
}
}

espm::Loader l = CreateEspmLoader();

int main(int argc, char* argv[])
{
  std::vector<const char*> args = { argv, argv + argc };

  if (l.GetFileNames().empty()) {
    args.push_back("~[espm]");
  }

  return Catch::Session().run(args.size(), args.data());
}
