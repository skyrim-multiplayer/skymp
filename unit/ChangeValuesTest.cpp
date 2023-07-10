#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "PacketParser.h"
#include "libespm/Loader.h"

PartOne& GetPartOne();
extern espm::Loader l;

using namespace std::chrono_literals;

TEST_CASE("ChangeValues packet is parsed correctly", "[ChangeValues]")
{
  class MyActionListener : public ActionListener
  {
  public:
    MyActionListener()
      : ActionListener(GetPartOne())
    {
    }

    void OnChangeValues(const RawMessageData& rawMsgData_,
                        const ActorValues& actorValues_) override
    {
      rawMsgData = rawMsgData_;
      actorValues = actorValues_;
    }

    RawMessageData rawMsgData;
    ActorValues actorValues;
  };

  nlohmann::json j{
    { "t", MsgType::ChangeValues },
    { "data", { { "health", 0.5 }, { "magicka", 0.3 }, { "stamina", 0 } } }
  };

  auto msg = MakeMessage(j);

  MyActionListener listener;

  PacketParser p;
  p.TransformPacketIntoAction(
    122, reinterpret_cast<Networking::PacketData>(msg.data()), msg.size(),
    listener);

  REQUIRE(listener.actorValues.healthPercentage == 0.5f);
  REQUIRE(listener.actorValues.magickaPercentage == 0.3f);
  REQUIRE(listener.actorValues.staminaPercentage == 0.0f);
}

TEST_CASE("Player attribute percentages are changing correctly",
          "[ChangeValues] ")
{
  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  ActorValues actorValues;
  actorValues.healthPercentage = 0.75f;
  actorValues.magickaPercentage = 0.f;
  actorValues.staminaPercentage = 0.7f;
  p.GetActionListener().OnChangeValues(msgData, actorValues);
  auto changeForm = ac.GetChangeForm();

  REQUIRE(changeForm.actorValues.healthPercentage == 0.75f);
  REQUIRE(changeForm.actorValues.magickaPercentage == 0.0f);
  REQUIRE(changeForm.actorValues.staminaPercentage == 0.7f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnChangeValues call is cropping percentage values",
          "[ChangeValues]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  uint32_t baseId = ac.GetBaseId();
  auto appearance = ac.GetAppearance();
  uint32_t raceId = appearance ? appearance->raceId : 0;
  BaseActorValues baseValues =
    GetBaseActorValues(&p.worldState, baseId, raceId);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  ActorValues actorValues;
  actorValues.healthPercentage = 0.1f;
  actorValues.magickaPercentage = 0.f;
  actorValues.staminaPercentage = 0.f;
  ac.SetPercentages(actorValues);
  auto past = std::chrono::steady_clock::now() - 1s;
  ac.SetLastAttributesPercentagesUpdate(past);
  actorValues.healthPercentage = 1.f;
  actorValues.magickaPercentage = 1.f;
  actorValues.staminaPercentage = 1.f;
  p.GetActionListener().OnChangeValues(msgData, actorValues);

  std::chrono::duration<float> elapsedTime =
    std::chrono::steady_clock::now() - past;

  float expectedHealth = baseValues.healRate * baseValues.healRateMult *
    elapsedTime.count() / 10000.0f;
  float expectedMagicka = baseValues.magickaRate * baseValues.magickaRateMult *
    elapsedTime.count() / 10000.0f;
  float expectedStamina = baseValues.staminaRate * baseValues.staminaRateMult *
    elapsedTime.count() / 10000.0f;

  auto changeForm = ac.GetChangeForm();

  REQUIRE_THAT(changeForm.actorValues.healthPercentage,
               Catch::Matchers::WithinAbs(expectedHealth + 0.1f, 0.001f));
  REQUIRE_THAT(changeForm.actorValues.staminaPercentage,
               Catch::Matchers::WithinAbs(expectedStamina, 0.001f));
  REQUIRE_THAT(changeForm.actorValues.magickaPercentage,
               Catch::Matchers::WithinAbs(expectedMagicka, 0.001f));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("ChangeValues message is being delivered to client",
          "[ChangeValues]")
{
  PartOne partOne;
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  partOne.Messages().clear();

  nlohmann::json j = nlohmann::json{ { "t", MsgType::ChangeValues },
                                     { "data",
                                       {
                                         { "health", 1.0f },
                                         { "magicka", 1.0f },
                                         { "stamina", 1.0f },
                                       } } };
  std::string s = MakeMessage(j);

  ac.SendToUser(s.data(), s.size(), true);

  REQUIRE(partOne.Messages().size() == 1);
  nlohmann::json message = partOne.Messages()[0].j;
  REQUIRE(message["data"]["health"] == 1.0f);
  REQUIRE(message["data"]["magicka"] == 1.0f);
  REQUIRE(message["data"]["stamina"] == 1.0f);

  partOne.DestroyActor(0xff000000);
  DoDisconnect(partOne, 0);
}

TEST_CASE("OnChangeValues function sends ChangeValues message with new "
          "percentages if input values was incorrect",
          "[ChangeValues]")
{
  using namespace std::chrono_literals;

  PartOne& partOne = GetPartOne();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  nlohmann::json j = nlohmann::json{
    { "t", MsgType::ChangeValues },
    { "data",
      { { "health", 1.0f }, { "magicka", 1.0f }, { "stamina", 1.0f } } }
  };
  ac.SetPercentages({ 0.0f, 0.0f, 0.0f });
  auto past = std::chrono::steady_clock::now() - 1s;
  ac.SetLastAttributesPercentagesUpdate(past);

  partOne.Messages().clear();
  DoMessage(partOne, 0, j);

  REQUIRE(partOne.Messages().size() == 1);
  nlohmann::json message = partOne.Messages()[0].j;

  REQUIRE(message["data"]["health"] != 0.0f);
  REQUIRE(message["data"]["health"] != 1.0f);
  REQUIRE(message["data"]["magicka"] != 0.0f);
  REQUIRE(message["data"]["magicka"] != 1.0f);
  REQUIRE(message["data"]["stamina"] != 0.0f);
  REQUIRE(message["data"]["stamina"] != 1.0f);

  partOne.DestroyActor(0xff000000);
  DoDisconnect(partOne, 0);
}

TEST_CASE("OnChangeValues function doesn't sends ChangeValues message if "
          "input values is ok",
          "[ChangeValues]")
{
  using namespace std::chrono_literals;

  PartOne& partOne = GetPartOne();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);
  partOne.Messages().clear();

  nlohmann::json j = nlohmann::json{
    { "t", MsgType::ChangeValues },
    { "data",
      { { "health", 1.0f }, { "magicka", 1.0f }, { "stamina", 1.0f } } }
  };
  ac.SetPercentages({ 1.0f, 1.0f, 1.0f });
  auto past = std::chrono::steady_clock::now() - 1s;
  ac.SetLastAttributesPercentagesUpdate(past);

  DoMessage(partOne, 0, j);

  REQUIRE(partOne.Messages().size() == 0);

  partOne.DestroyActor(0xff000000);
  DoDisconnect(partOne, 0);
}
