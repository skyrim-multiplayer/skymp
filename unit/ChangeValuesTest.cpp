#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "PacketParser.h"

PartOne& GetPartOne();

TEST_CASE("ChangeValues packet is parsed correctly", "[ChangeValues]")
{
  class MyActionListener : public IActionListener
  {
  public:
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

  IActionListener::RawMessageData msgData;
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
  auto look = ac.GetLook();
  uint32_t raceId = look ? look->raceId : 0;
  BaseActorValues baseValues = GetBaseActorValues(baseId, raceId);

  IActionListener::RawMessageData msgData;
  msgData.userId = 0;

  ac.SetPercentages(0.0f, 0.0f, 0.0f);
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
               Catch::Matchers::WithinAbs(expectedHealth, 0.000001f));
  REQUIRE_THAT(changeForm.magickaPercentage,
               Catch::Matchers::WithinAbs(expectedMagicka, 0.000001f));
  REQUIRE_THAT(changeForm.staminaPercentage,
               Catch::Matchers::WithinAbs(expectedStamina, 0.000001f));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
