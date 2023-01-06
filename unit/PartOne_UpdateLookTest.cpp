#include "TestUtils.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("SetRaceMenuOpen failures", "[PartOne]")
{

  PartOne partOne;

  partOne.CreateActor(0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c);

  REQUIRE_THROWS_WITH(
    partOne.SetRaceMenuOpen(0xff000000, true),
    ContainsSubstring(
      "Actor with id 0xff000000 is not attached to any of users"));

  REQUIRE_THROWS_WITH(
    partOne.SetRaceMenuOpen(0xffffffff, true),
    ContainsSubstring("Form with id 0xffffffff doesn't exist"));

  partOne.worldState.AddForm(std::make_unique<MpForm>(), 0xffffffff);

  REQUIRE_THROWS_WITH(
    partOne.SetRaceMenuOpen(0xffffffff, true),
    ContainsSubstring("Form with id 0xffffffff is not Actor"));
}

TEST_CASE("SetRaceMenuOpen", "[PartOne]")
{

  PartOne partOne;

  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  auto actor = std::dynamic_pointer_cast<MpActor>(
    partOne.worldState.LookupFormById(0xff000000));
  partOne.SetUserActor(1, 0xff000000);
  partOne.Messages().clear();

  REQUIRE(actor->IsRaceMenuOpen() == false);

  partOne.SetRaceMenuOpen(0xff000000, true);

  REQUIRE(actor->IsRaceMenuOpen() == true);
  REQUIRE(partOne.Messages().size() == 1);
  REQUIRE(partOne.Messages()[0].j ==
          nlohmann::json{ { "type", "setRaceMenuOpen" }, { "open", true } });
  REQUIRE(partOne.Messages()[0].userId == 1);
  REQUIRE(partOne.Messages()[0].reliable);

  for (int i = 0; i < 3; ++i)
    partOne.SetRaceMenuOpen(0xff000000, true);
  REQUIRE(partOne.Messages().size() == 1);

  partOne.SetRaceMenuOpen(0xff000000, false);
  REQUIRE(partOne.Messages().size() == 2);
  REQUIRE(partOne.Messages()[1].j ==
          nlohmann::json{ { "type", "setRaceMenuOpen" }, { "open", false } });
  REQUIRE(partOne.Messages()[1].userId == 1);
  REQUIRE(partOne.Messages()[1].reliable);

  for (int i = 0; i < 3; ++i)
    partOne.SetRaceMenuOpen(0xff000000, false);
  REQUIRE(partOne.Messages().size() == 2);
}

TEST_CASE("Appearance <=> JSON casts", "[PartOne]")
{
  simdjson::dom::parser p;
  auto jAppearanceSimd = p.parse(jAppearance["data"].dump());
  auto appearance = Appearance::FromJson(jAppearanceSimd.value());

  REQUIRE(nlohmann::json::parse(appearance.ToJson()) == jAppearance["data"]);
}

TEST_CASE("UpdateAppearance1", "[PartOne]")
{

  PartOne partOne;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  partOne.SetRaceMenuOpen(0xff000ABC, true);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xffABCABC);

  partOne.Messages().clear();
  auto doAppearance = [&] { DoMessage(partOne, 0, jAppearance); };
  doAppearance();

  REQUIRE(partOne.Messages().size() == 2);
  REQUIRE(std::find_if(partOne.Messages().begin(), partOne.Messages().end(),
                       [&](auto m) {
                         return m.j["t"] == MsgType::UpdateAppearance &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jAppearance["data"];
                       }) != partOne.Messages().end());

  auto& ac = partOne.worldState.GetFormAt<MpActor>(0xff000ABC);
  REQUIRE(ac.GetAppearance() != nullptr);
  REQUIRE(nlohmann::json::parse(ac.GetAppearanceAsJson()) ==
          jAppearance["data"]);
  REQUIRE(ac.IsRaceMenuOpen() == false);
}

TEST_CASE("UpdateAppearance2", "[PartOne]")
{

  PartOne partOne;

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  partOne.SetRaceMenuOpen(0xff000ABC, true);

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xffABCABC);

  partOne.Messages().clear();
  auto doAppearance = [&] { DoMessage(partOne, 0, jAppearance); };
  doAppearance();

  REQUIRE(partOne.Messages().size() == 2);
  REQUIRE(std::find_if(partOne.Messages().begin(), partOne.Messages().end(),
                       [&](auto m) {
                         return m.j["t"] == MsgType::UpdateAppearance &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jAppearance["data"];
                       }) != partOne.Messages().end());

  REQUIRE(partOne.worldState.GetFormAt<MpActor>(0xff000ABC).GetAppearance() !=
          nullptr);
  REQUIRE(
    nlohmann::json::parse(partOne.worldState.GetFormAt<MpActor>(0xff000ABC)
                            .GetAppearanceAsJson()) == jAppearance["data"]);
}
