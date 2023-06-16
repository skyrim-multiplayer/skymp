#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "HeuristicPolicy.h"
#include "MpActor.h"
#include "MpFormGameObject.h"

TEST_CASE("HeuristicPolicy", "[HeuristicPolicy]")
{
  auto logger = std::make_shared<spdlog::logger>("empty logger");
  WorldState wst;
  HeuristicPolicy policy(logger, &wst);

  MpActor actor(LocationalData(), FormCallbacks::DoNothing());
  MpFormGameObject actorGameObject(&actor);
  VarValue actorVarValue(&actorGameObject);

  std::vector<VarValue> args = { actorVarValue };

  REQUIRE_THROWS_WITH(policy.GetDefaultActor("", "", 0),
                      Catch::Matchers::ContainsSubstring(
                        "Invalid stackId was passed to GetDefaultActor (0)"));

  policy.SetDefaultActor(0, nullptr);
  REQUIRE(policy.GetDefaultActor("", "", 0) == nullptr);

  policy.BeforeSendPapyrusEvent(nullptr, "OnACTivatE", args.data(),
                                args.size(), 0);

  REQUIRE(policy.GetDefaultActor("", "", 0) == &actor);
}
