#include "MsgType.h"
#include <MpActor.h>
#include <MpForm.h>
#include <WorldState.h>
#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

using namespace Catch;

TEST_CASE("AddForm failures", "[WorldState]")
{
  WorldState worldState;
  worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000);
  REQUIRE_THROWS_WITH(
    worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0xff000000),
    Contains("Form with id ff000000 already exists"));
}

TEST_CASE("DestroyForm failures", "[WorldState]")
{
  WorldState worldState;
  REQUIRE_THROWS_WITH(worldState.DestroyForm(0x12345678),
                      Contains("Form with id 12345678 doesn't exist"));

  worldState.AddForm(std::unique_ptr<MpForm>(new MpForm), 0x12345678);
  REQUIRE_THROWS_WITH(
    worldState.DestroyForm<MpActor>(0x12345678),
    Contains("Expected form 12345678 to be Actor, but got Form"));
}