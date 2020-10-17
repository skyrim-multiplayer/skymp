#define CATCH_CONFIG_MAIN
#include <Loader.h>
#include <catch2/catch.hpp>
#include <iostream>

#include <sqlite_orm.h>

#ifndef WIN32
#  define DATA_DIR "/skyrim_data_dir"
#else
#  define DATA_DIR                                                            \
    "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "      \
    "Edition\\Data"
#endif

namespace {
inline void OnProgress(std::string fileName, float readDur, float parseDur,
                       uintmax_t fileSize)
{
  std::cout << "[ESPM] " << fileName << " read in " << readDur
            << "s, parsed in " << parseDur << "s, size is "
            << (fileSize / 1024 / 1024) << "Mb" << std::endl;
}
}

espm::Loader l(DATA_DIR,
               { "Skyrim.esm", "Update.esm", "Dawnguard.esm",
                 "HearthFires.esm", "Dragonborn.esm" },
               OnProgress);