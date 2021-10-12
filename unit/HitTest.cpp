#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "ActionListener.h"
#include "ActionListener.cpp"

using Catch::Matchers::Contains;

PartOne& GetPartOne();
extern espm::Loader l;

TEST_CASE("OnHit throws exception if actor isn't attached", "[OnHit]")
{
  auto& listener = GetPartOne().GetActionListener();
  HitData hitData;
  IActionListener::RawMessageData rawMessageData;

  REQUIRE_THROWS_WITH(listener.OnHit(rawMessageData, hitData),
                      "Unable to change values without Actor attached");
}
