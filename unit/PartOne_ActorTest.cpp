#include "TestUtils.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("CreateActor/DestroyActor", "[PartOne]")
{

  PartOne partOne;

  // Create:

  REQUIRE(!partOne.worldState.LookupFormById(0xff000ABC));
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);

  auto form = partOne.worldState.LookupFormById(0xff000ABC).get();
  REQUIRE(form);
  REQUIRE(form->GetFormId() == 0xff000ABC);

  auto ac = dynamic_cast<MpActor*>(form);
  REQUIRE(ac);
  REQUIRE(ac->GetPos() == NiPoint3{ 1.f, 2.f, 3.f });
  REQUIRE(ac->GetAngle() == NiPoint3{ 0.f, 0.f, 180.f });
  REQUIRE(ac->GetCellOrWorld() == FormDesc::Tamriel());

  // Destroy:

  partOne.DestroyActor(0xff000ABC);
  REQUIRE(!partOne.worldState.LookupFormById(0xff000ABC));
}

TEST_CASE("SetUserActor", "[PartOne]")
{

  PartOne partOne;
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  DoConnect(partOne, 0);

  REQUIRE(!partOne.GetUserActor(0));
  partOne.SetUserActor(0, 0xff000ABC);
  REQUIRE(partOne.GetUserActor(0) == 0xff000ABC);
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages().at(0).j.dump() ==
          nlohmann::json{
            { "type", "createActor" },
            { "refrId", 0xff000ABC },
            { "idx", 0 },
            { "isMe", true },
            { "props",
              nlohmann::json{ { "healRate", 0.7 },
                              { "healRateMult", 100.f },
                              { "health", 100.f },
                              { "isHostedByOther", true },
                              { "learnedSpells", nlohmann::json::array() },
                              { "magicka", 100.f },
                              { "magickaRate", 3.f },
                              { "magickaRateMult", 100.f },
                              { "stamina", 100.f },
                              { "staminaRate", 5.f },
                              { "staminaRateMult", 100.f },
                              { "healthPercentage", 1.f },
                              { "staminaPercentage", 1.f },
                              { "magickaPercentage", 1.f } } },
            { "transform",
              nlohmann::json{ { "pos", { 1.f, 2.f, 3.f } },
                              { "rot", { 0.f, 0.f, 180.f } },
                              { "worldOrCell", 0x3c } } } }
            .dump());

  // Trying to destroy actor:
  partOne.DestroyActor(0xff000ABC);
  REQUIRE(!partOne.GetUserActor(0));

  // TODO: More manipulations with actor transfer
}

TEST_CASE("Use SetUserActor with 0 formId argument", "[PartOne]")
{

  PartOne partOne;
  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);

  REQUIRE(partOne.GetUserActor(1) == 0);
  partOne.SetUserActor(1, 0xff000ABC);
  REQUIRE(partOne.GetUserActor(1) == 0xff000ABC);
  partOne.SetUserActor(1, 0);
  REQUIRE(partOne.GetUserActor(1) == 0);
}

TEST_CASE("SetUserActor failures", "[PartOne]")
{
  PartOne partOne;
  REQUIRE_THROWS_WITH(partOne.SetUserActor(9, 0xff000000),
                      ContainsSubstring("User with id 9 doesn't exist"));
  DoConnect(partOne, 9);

  REQUIRE_THROWS_WITH(
    partOne.SetUserActor(9, 0xff000000),
    ContainsSubstring("Form with id 0xff000000 doesn't exist"));

  partOne.worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);

  REQUIRE_THROWS_WITH(
    partOne.SetUserActor(9, 0xff000000),
    ContainsSubstring("Form with id 0xff000000 is not Actor"));
}

TEST_CASE("createActor message contains Appearance", "[PartOne]")
{

  PartOne partOne;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  const Appearance appearance = Appearance::FromJson(jAppearance["data"]);
  partOne.worldState.GetFormAt<MpActor>(0xff000ABC).SetAppearance(&appearance);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000FFF, { 100.f, 200.f, 300.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xff000FFF);

  REQUIRE(std::find_if(partOne.Messages().begin(), partOne.Messages().end(),
                       [&](auto m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["appearance"] == jAppearance["data"];
                       }) != partOne.Messages().end());

  /*REQUIRE_THROWS_WITH(
    doAppearance(), ContainsSubstring("Unable to update appearance, RaceMenu is
  not open"));

  partOne.SetRaceMenuOpen(0xff000ABC, true);
  doAppearance();*/
}

TEST_CASE("GetUserActor", "[PartOne]")
{

  PartOne partOne;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000000);

  REQUIRE(partOne.GetUserActor(0) == 0xff000000);
  REQUIRE(partOne.serverState.ActorByUser(0) != nullptr);
  REQUIRE(partOne.serverState.UserByActor(&ac) == 0);

  DoDisconnect(partOne, 0);

  REQUIRE(partOne.serverState.ActorByUser(0) == nullptr);
  REQUIRE(partOne.serverState.UserByActor(&ac) == Networking::InvalidUserId);
  REQUIRE_THROWS_WITH(partOne.GetUserActor(0),
                      ContainsSubstring("User with id 0 doesn't exist"));
}

TEST_CASE("Destroying actor in disconnect event handler", "[PartOne]")
{

  static PartOne partOne;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  DoMessage(partOne, 0, jMovement);

  static auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000ABC);

  class Listener : public PartOne::Listener
  {
  public:
    void OnConnect(Networking::UserId userId) override {}
    void OnDisconnect(Networking::UserId userId) override
    {
      REQUIRE(partOne.serverState.UserByActor(&ac) == 0);
      partOne.DestroyActor(0xff000ABC);
    }
    void OnCustomPacket(Networking::UserId userId,
                        const simdjson::dom::element& content) override
    {
    }
    bool OnMpApiEvent(const char* eventName,
                      std::optional<simdjson::dom::element> args,
                      std::optional<uint32_t> formId) override
    {
      return true;
    }
  };

  partOne.AddListener(std::shared_ptr<PartOne::Listener>(new Listener));

  REQUIRE(partOne.serverState.UserByActor(&ac) == 0);
  DoDisconnect(partOne, 1);
  REQUIRE(partOne.serverState.UserByActor(&ac) == Networking::InvalidUserId);
}

TEST_CASE("Bug with subscription", "[PartOne]")
{

  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetEnabled(0xff000000, false);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetEnabled(0xff000000, false);
  partOne.SetEnabled(0xff000000, true);
  partOne.SetUserActor(0, 0xff000000);

  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["type"] == "createActor");
}

TEST_CASE("SetUserActor doesn't work with disabled actors", "[PartOne]")
{
  PartOne partOne;

  REQUIRE_THROWS_WITH(partOne.GetUserActor(Networking::InvalidUserId),
                      ContainsSubstring("User with id 65535 doesn't exist"));

  REQUIRE_THROWS_WITH(partOne.SetUserActor(Networking::InvalidUserId, 0),
                      ContainsSubstring("User with id 65535 doesn't exist"));
}

TEST_CASE("Actor should see its inventory in 'createActor' message",
          "[PartOne]")
{

  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c);
  partOne.worldState.GetFormAt<MpActor>(0xff000000).AddItem(0x12eb7, 3);
  partOne.SetUserActor(0, 0xff000000);

  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["type"] == "createActor");
  REQUIRE(partOne.Messages()[0].j["props"]["inventory"] ==
          Inventory().AddItem(0x12eb7, 3).ToJson());
}

TEST_CASE("'isRaceMenuOpen' property should present in 'createActor'",
          "[PartOne]")
{

  PartOne partOne;
  DoConnect(partOne, 0);

  partOne.CreateActor(0xff000000, { 1, 1, 1 }, 3, 0x3c);
  partOne.worldState.GetFormAt<MpActor>(0xff000000).SetRaceMenuOpen(true);
  partOne.SetUserActor(0, 0xff000000);

  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j["type"] == "createActor");
  REQUIRE(partOne.Messages()[0].j["props"]["isRaceMenuOpen"] == true);
}
