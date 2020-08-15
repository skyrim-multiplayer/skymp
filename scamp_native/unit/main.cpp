#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <espm.h>

#include <Loader.h>

#define DATA_DIR                                                              \
  "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special "        \
  "Edition\\Data"

espm::Loader l(DATA_DIR,
               { "Skyrim.esm", "Update.esm", "Dawnguard.esm",
                 "HearthFires.esm", "Dragonborn.esm" });