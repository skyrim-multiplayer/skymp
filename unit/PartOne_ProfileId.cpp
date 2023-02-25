#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE("Should return 0 for invalid/unexisting profileId", "[ProfileId]")
{
  PartOne p;
  REQUIRE(p.GetActorsByProfileId(-1).empty());
  REQUIRE(p.GetActorsByProfileId(0).empty());
}

TEST_CASE("Should find a correct actor by profileId", "[ProfileId]")
{

  PartOne p;
  REQUIRE(p.GetActorsByProfileId(20).empty());
  p.CreateActor(0xff000000, { 1, 2, 3 }, 4, 0x3c, ProfileId(20));
  REQUIRE(p.GetActorsByProfileId(20) == std::set<uint32_t>({ 0xff000000 }));
  p.CreateActor(0xff000001, { 1, 2, 3 }, 4, 0x3c, ProfileId(20));
  REQUIRE(p.GetActorsByProfileId(20) ==
          std::set<uint32_t>({ 0xff000000, 0xff000001 }));
}
