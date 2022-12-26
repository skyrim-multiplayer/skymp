#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <limits>

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("Loading Cells from Solstheim.esm", "[LoadCells]")
{
  auto& p = GetPartOne();
  auto& t = p.worldState.GetReferencesAtPosition(0x04000800, 7, 8);
  REQUIRE(t.size());
}

TEST_CASE("Check number of messages if player spawned in Skyrim",
          "[LoadCells]")
{
  auto& p = GetPartOne();

  DoConnect(p, 0);
  p.Messages().clear();
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);

  REQUIRE(p.Messages().size() > 1);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("Check number of messages if player spawned in Solstheim",
          "[LoadCells]")
{
  auto& p = GetPartOne();

  DoConnect(p, 0);
  p.Messages().clear();
  p.CreateActor(0xff000000, { 7 * 4096, 8 * 4096, 0 }, 0, 0x04000800);
  p.SetUserActor(0, 0xff000000);

  REQUIRE(p.Messages().size() > 1);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
