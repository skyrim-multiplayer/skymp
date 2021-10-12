#include "TestUtils.hpp"
#include <catch2/catch.hpp>
#include <chrono>

#include "GetBaseActorValues.h"
#include "Loader.h"
#include "PacketParser.h"

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("Hit packet is received successfully", "[Hit]")
{
  PartOne& partOne = GetPartOne();
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);

  IActionListener::RawMessageData rawMessageData;
  rawMessageData.userId = 0;
  HitData hitData;
  hitData.target = 0x14;

  partOne.Messages().clear();
  partOne.GetActionListener().OnHit(rawMessageData, hitData);

  REQUIRE(partOne.Messages().size() == 1);

}