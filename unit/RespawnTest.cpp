#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("DeathState packed is correct if actor was killed", "[Respawn]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  p.Messages().clear();
  ac.Kill();
  REQUIRE(p.Messages().size() == 1);
  nlohmann::json message = p.Messages()[0].j;
  REQUIRE(message["t"] == MsgType::DeathStateContainer);

  nlohmann::json updateProperyMsg = message["tIsDead"];
  nlohmann::json teleportMsg = message["tTeleport"];
  nlohmann::json changeValuesMsg = message["tChangeValues"];

  REQUIRE(updateProperyMsg["t"] == MsgType::UpdateProperty);
  REQUIRE(updateProperyMsg["propName"] == "isDead");
  REQUIRE(updateProperyMsg["data"] == true);
  REQUIRE(updateProperyMsg["idx"] == ac.GetIdx());
  REQUIRE(teleportMsg.is_null());
  REQUIRE(changeValuesMsg.is_null());

  REQUIRE(ac.IsDead());
  REQUIRE(ac.GetChangeForm().actorValues.healthPercentage == 0.f);
}

TEST_CASE("DeathState packed is correct if actor is respawning", "[Respawn]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.SetPos(ac.GetSpawnPoint().pos);
  ac.SetAngle(ac.GetSpawnPoint().rot);
  ac.SetCellOrWorld(ac.GetSpawnPoint().cellOrWorldDesc);

  ac.Kill();
  p.Messages().clear();
  ac.Respawn();

  REQUIRE(p.Messages().size() == 2);

  nlohmann::json message = p.Messages()[0].j;
  REQUIRE(message["t"] == MsgType::DeathStateContainer);

  nlohmann::json updateProperyMsg = message["tIsDead"];
  nlohmann::json teleportMsg = message["tTeleport"];
  nlohmann::json changeValuesMsg = message["tChangeValues"];

  REQUIRE(updateProperyMsg["t"] == MsgType::UpdateProperty);
  REQUIRE(updateProperyMsg["propName"] == "isDead");
  REQUIRE(updateProperyMsg["data"] == false);
  REQUIRE(updateProperyMsg["idx"] == ac.GetIdx());

  REQUIRE(teleportMsg["t"] == MsgType::Teleport);
  REQUIRE(changeValuesMsg["t"] == MsgType::ChangeValues);

  REQUIRE(ac.IsDead() == false);
  REQUIRE(ac.GetChangeForm().actorValues.healthPercentage == 1.f);

  // TODO: should probably not sending to ourselves. see also MpActor::Respawn
  nlohmann::json message2 = p.Messages()[1].j;
  REQUIRE(message2["t"] == MsgType::UpdateProperty);

  REQUIRE(message2["propName"] == "isDead");
  REQUIRE(message2["data"] == false);
  REQUIRE(message2["idx"] == ac.GetIdx());
  REQUIRE(message2["refrId"] == ac.GetFormId());
  REQUIRE(message2["baseRecordType"] == nlohmann::json{});
}
