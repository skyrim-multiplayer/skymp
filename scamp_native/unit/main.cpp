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

std::string dataDir = DATA_DIR;

#include "ActorTest.h"
#include "Benchmarks.h"
#include "ConsoleCommandTest.h"
#include "CraftTest.h"
#include "EspmTest.h"
#include "FormDescTest.h"
#include "GridTest.h"
#include "Grid_MoveTest.h"
#include "HeuristicPolicyTest.h"
#include "IdManagerTest.h"
#include "LeveledListUtilsTest.h"
#include "MigrationDatabaseTest.h"
#include "MovementValidationTest.h"
#include "NetworkingTest.h"
#include "Networking_CombinedTest.h"
#include "Networking_DataTransferTest.h"
#include "Networking_HandlePacketClientsideTest.h"
#include "Networking_HandlePacketServersideTest.h"
#include "Networking_MockTest.h"
#include "NpcExists.h"
#include "ObjectReferenceTest.h"
#include "PapyrusCompatibilityTest.h"
#include "PapyrusDebugTest.h"
#include "PapyrusFormListTest.h"
#include "PapyrusFormTest.h"
#include "PapyrusObjectReferenceTest.h"
#include "PapyrusSkympTest.h"
#include "PapyrusUtilityTest.h"
#include "PartOneTest.h"
#include "PartOne_ActivateTest.h"
#include "PartOne_ActorTest.h"
#include "PartOne_BotTest.h"
#include "PartOne_IdGen.h"
#include "PartOne_MovementTest.h"
#include "PartOne_ProfileId.h"
#include "PartOne_UpdateEquipmentTest.h"
#include "PartOne_UpdateLookTest.h"
#include "PrimitiveTest.h"
#include "SaveStorageTest.h"
#include "ServerStateTest.h"
#include "VarValueTest.h"
#include "VirtualMachineTest.h"
#include "WorldStateTest.h"