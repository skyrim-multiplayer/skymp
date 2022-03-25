#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "Loader.h"
#include "PacketParser.h"

PartOne& GetPartOne();
extern espm::Loader l;

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
                        const float healthPercentage_,
                        const float magickaPercentage_,
                        const float staminaPercentage_) override
    {
      rawMsgData = rawMsgData_;
      healthPercentage = healthPercentage_;
      magickaPercentage = magickaPercentage_;
      staminaPercentage = staminaPercentage_;
    }

    RawMessageData rawMsgData;
    float healthPercentage = 1;
    float magickaPercentage = 1;
    float staminaPercentage = 1;
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

  REQUIRE(listener.healthPercentage == 0.5f);
  REQUIRE(listener.magickaPercentage == 0.3f);
  REQUIRE(listener.staminaPercentage == 0.0f);
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

  p.GetActionListener().OnChangeValues(msgData, 0.75f, 0.0f, 0.7f);
  auto changeForm = ac.GetChangeForm();

  REQUIRE(changeForm.healthPercentage == 0.75f);
  REQUIRE(changeForm.magickaPercentage == 0.0f);
  REQUIRE(changeForm.staminaPercentage == 0.7f);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("OnChangeValues call is cropping percentage values",
          "[ChangeValues]")
{
  using namespace std::chrono_literals;

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

  ac.SetPercentages(0.1f, 0.0f, 0.0f);
  auto past = std::chrono::steady_clock::now() - 1s;
  ac.SetLastAttributesPercentagesUpdate(past);
  p.GetActionListener().OnChangeValues(msgData, 1.0f, 1.0f, 1.0f);

  auto now = ac.GetLastAttributesPercentagesUpdate();
  std::chrono::duration<float> timeDuration = now - past;
  float time = timeDuration.count();

  float expectedHealth =
    baseValues.healRate * baseValues.healRateMult * time / 10000.0f;
  float expectedMagicka =
    baseValues.magickaRate * baseValues.magickaRateMult * time / 10000.0f;
  float expectedStamina =
    baseValues.staminaRate * baseValues.staminaRateMult * time / 10000.0f;

  auto changeForm = ac.GetChangeForm();

  REQUIRE_THAT(changeForm.healthPercentage,
               Catch::Matchers::WithinAbs(expectedHealth + 0.1f, 0.000001f));
  REQUIRE_THAT(changeForm.magickaPercentage,
               Catch::Matchers::WithinAbs(expectedMagicka, 0.000001f));
  REQUIRE_THAT(changeForm.staminaPercentage,
               Catch::Matchers::WithinAbs(expectedStamina, 0.000001f));

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

  nlohmann::json j = nlohmann::json{
    { "t", MsgType::ChangeValues },
    { "data",
      { { "health", 1.0f }, { "magicka", 1.0f }, { "stamina", 1.0f } } }
  };
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
  ac.SetPercentages(0.0f, 0.0f, 0.0f);
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
  ac.SetPercentages(1.0f, 1.0f, 1.0f);
  auto past = std::chrono::steady_clock::now() - 1s;
  ac.SetLastAttributesPercentagesUpdate(past);

  DoMessage(partOne, 0, j);

  REQUIRE(partOne.Messages().size() == 0);

  partOne.DestroyActor(0xff000000);
  DoDisconnect(partOne, 0);
}
