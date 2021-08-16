#define CATCH_CONFIG_RUNNER
#include "MiniDump.h"
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
  Tools::CreateMiniDumpOnUnHandledException();

  std::vector<const char*> args = { argv, argv + argc };

  if (l.GetFileNames().empty()) {
    args.push_back("~[espm]");
  }

  return Catch::Session().run(args.size(), args.data());
}

#include "ActorTest.h"
#include "ActorsMapTest.h"
#include "Benchmarks.h"
#include "ConsoleCommandTest.h"
#include "CraftTest.h"
#include "DistContentsTest.h"
#include "EspmTest.h"
#include "FormDescTest.h"
#include "GridTest.h"
#include "Grid_MoveTest.h"
#include "HeuristicPolicyTest.h"
#include "HttpClientApiTest.h"
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
