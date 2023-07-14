#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "HeuristicPolicy.h"
#include "PapyrusUtility.h"

TEST_CASE("Wait", "[Papyrus][Utility]")
{
  WorldState wst;
  PapyrusUtility utility;
  std::shared_ptr<spdlog::logger> logger;
  utility.compatibilityPolicy.reset(new HeuristicPolicy(logger, &wst));

  bool waitFinished = false;

  utility.Wait(VarValue::None(), { VarValue(0.03f) }).Then([&](VarValue) {
    waitFinished = true;
  });
  REQUIRE(!waitFinished);

  wst.Tick();
  REQUIRE(!waitFinished);

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  wst.Tick();
  REQUIRE(waitFinished);
}

TEST_CASE("RandomInt", "[Papyrus][Utility]")
{
  WorldState wst;
  PapyrusUtility utility;
  int32_t low = 0, high = 100;
  int32_t res = static_cast<int32_t>(utility.RandomInt(VarValue::None(), {}));
  bool good = low <= res && res <= high;
  REQUIRE(good);
  low = 253;
  high = 666;
  res = static_cast<int32_t>(
    utility.RandomInt(VarValue::None(), { VarValue(low), VarValue(high) }));
  good = low <= res && res <= high;
  REQUIRE(good);
  low = 13;
  high = 100;
  res = static_cast<int32_t>(
    utility.RandomInt(VarValue::None(), { VarValue(low) }));
  good = low <= res && res <= high;
  REQUIRE(good);
}
