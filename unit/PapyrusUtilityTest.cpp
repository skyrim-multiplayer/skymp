#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "script_classes/PapyrusUtility.h"
#include "script_compatibility_policies/HeuristicPolicy.h"

TEST_CASE("Wait", "[Papyrus][Utility]")
{
  WorldState wst;
  PapyrusUtility utility;
  std::shared_ptr<spdlog::logger> logger;
  utility.compatibilityPolicy.reset(new HeuristicPolicy(&wst));

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
  int32_t low = 253, high = 666;
  auto res = static_cast<int32_t>(
    utility.RandomInt(VarValue::None(), { VarValue(low), VarValue(high) }));
  bool good = low <= res && res <= high;
  REQUIRE(good);
}
