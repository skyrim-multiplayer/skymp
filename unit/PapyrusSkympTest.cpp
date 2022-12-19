#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "HeuristicPolicy.h"
#include "PapyrusSkymp.h"

TEST_CASE("SetDefaultActor should store actor per stack", "[Papyrus][Skymp]")
{
  PartOne p;
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  auto logger = std::make_shared<spdlog::logger>("empty logger");
  PapyrusSkymp skymp;
  skymp.policy = std::make_shared<HeuristicPolicy>(logger, &p.worldState);

  skymp.SetDefaultActor(VarValue::AttachTestStackId(VarValue::None(), 0),
                        { ac.ToVarValue() });

  skymp.SetDefaultActor(VarValue::AttachTestStackId(VarValue::None(), 1),
                        { VarValue::None() });

  REQUIRE(skymp.policy->GetDefaultActor("", "", 0) == &ac);
  REQUIRE(skymp.policy->GetDefaultActor("", "", 1) == nullptr);
  REQUIRE_THROWS_WITH(
    skymp.policy->GetDefaultActor("", "", 91298),
    Catch::Matchers::ContainsSubstring(
      "Invalid stackId was passed to GetDefaultActor (91298)"));
  REQUIRE_THROWS_WITH(skymp.policy->GetDefaultActor("", "", -1),
                      Catch::Matchers::ContainsSubstring(
                        "Invalid stackId was passed to GetDefaultActor (-1)"));
}
