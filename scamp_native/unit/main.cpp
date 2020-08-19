#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <espm.h>

#include <Loader.h>

#ifndef WIN32
#  define DATA_DIR = "/skyrim_data_dir";
#else
#  define DATA_DIR                                                            \
    "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "      \
    "Edition\\Data"
#endif

espm::Loader l(DATA_DIR,
               { "Skyrim.esm", "Update.esm", "Dawnguard.esm",
                 "HearthFires.esm", "Dragonborn.esm" });