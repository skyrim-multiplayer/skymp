#include <catch2/catch_all.hpp>
#include <iostream>
#include <libespm/Loader.h>

#include "TestUtils.hpp"

namespace {
inline void OnProgress(const std::string& fileName, float readDuration,
                       float parseDuration, uintmax_t fileSize)
{
  std::cout << "[ESPM] " << fileName << " read in " << readDuration
            << "s, parsed in " << parseDuration << "s, size is "
            << (fileSize / 1024 / 1024) << "Mb" << std::endl;
}

inline espm::Loader CreateEspmLoader()
{
  try {
    std::vector<std::filesystem::path> files = { "Skyrim.esm", "Update.esm",
                                                 "Dawnguard.esm",
                                                 "HearthFires.esm",
                                                 "Dragonborn.esm" };

    std::filesystem::path dataDir = GetDataDir();

    std::filesystem::path skyrimEsm = dataDir / "Skyrim.esm";
    if (!std::filesystem::exists(skyrimEsm)) {
      files.clear();
      dataDir = std::filesystem::current_path();
      std::cout << skyrimEsm << " doesn't exist" << std::endl;
      std::cout << "Skipping tests with [espm] tag" << std::endl;
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
