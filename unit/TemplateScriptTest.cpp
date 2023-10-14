#include "MsgType.h"
#include "ServerState.h"
#include "TestUtils.hpp"
#include "script_storages/IScriptStorage.h"
#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

PartOne& GetPartOne();

namespace {
class MyScriptStorage : public IScriptStorage
{
  std::vector<uint8_t> GetScriptPex(const char* scriptName) override
  {
    if (scriptName == std::string("masterambushscript")) {
      throw std::runtime_error("OK");
    }
    return {};
  }

  const std::set<CIString>& ListScripts(bool forceReloadScripts) override
  {
    static const std::set<CIString> kSet = { "masterambushscript" };
    return kSet;
  }
};
}

TEST_CASE("MS13FrostbiteSpiderREF in BleakFalls should have scripts "
          "derived from NPC template",
          "[TemplateScript]")
{
  PartOne& p = GetPartOne();

  p.worldState.npcSettings["Skyrim.esm"].spawnInInterior = true;
  p.worldState.npcEnabled = true;

  p.worldState.AttachScriptStorage(std::make_shared<MyScriptStorage>());

  std::string what;
  try {
    uint32_t actorId = 0x3a1e1;
    auto& spider = p.worldState.GetFormAt<MpActor>(actorId);
    spider.SendPapyrusEvent("OnLoad");
  } catch (std::exception& e) {
    what = e.what();
  }

  REQUIRE(what == "OK");
}
