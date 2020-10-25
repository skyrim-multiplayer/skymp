#include "TestUtils.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Should generate id when zero id is passed to CreateActor",
          "[IdGen]")
{
  FakeSendTarget tgt;
  PartOne p;

  uint32_t actorId = p.CreateActor(0, { 1, 2, 3 }, 4, 0x3c, &tgt, -1);

  REQUIRE(p.worldState.LookupFormById(actorId) != nullptr);
  REQUIRE(actorId == 0xff000000);
}

TEST_CASE("Should not generate duplicates", "[IdGen]")
{
  FakeSendTarget tgt;
  PartOne p;

  p.CreateActor(0xff000000, { 1, 2, 3 }, 4, 0x3c, &tgt, -1);
  p.CreateActor(0xff000001, { 1, 2, 3 }, 4, 0x3c, &tgt, -1);

  uint32_t actorId = p.CreateActor(0, { 1, 2, 3 }, 4, 0x3c, &tgt, -1);

  REQUIRE(p.worldState.LookupFormById(actorId) != nullptr);
  REQUIRE(actorId == 0xff000002);
}