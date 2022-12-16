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
