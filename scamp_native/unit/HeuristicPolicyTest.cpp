#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "HeuristicPolicy.h"
#include "MpActor.h"
#include "MpFormGameObject.h"

TEST_CASE("HeuristicPolicy", "[HeuristicPolicy]")
{
  auto logger = std::make_shared<spdlog::logger>("empty logger");
  WorldState wst;
  HeuristicPolicy policy(logger);

  MpActor actor(LocationalData(), FormCallbacks::DoNothing());
  MpFormGameObject actorGameObject(&actor);
  VarValue actorVarValue(&actorGameObject);

  std::vector<VarValue> args = { actorVarValue };

  REQUIRE(policy.GetDefaultActor("", "") == nullptr);

  policy.BeforeSendPapyrusEvent(nullptr, "OnACTivatE", args.data(),
                                args.size());

  REQUIRE(policy.GetDefaultActor("", "") == &actor);
}