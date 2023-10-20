#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "script_classes/PapyrusDebug.h"
#include "script_compatibility_policies/HeuristicPolicy.h"

TEST_CASE("Notification", "[Papyrus][Debug]")
{

  PartOne p;
  {
    auto ac =
      std::make_unique<MpActor>(LocationalData(), p.CreateFormCallbacks());
    p.worldState.AddForm(std::move(ac), 0xff000000);
  }

  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  auto policy = new HeuristicPolicy(&p.worldState);
  policy->SetDefaultActor(VarValue::AttachTestStackId().GetMetaStackId(), &ac);

  PapyrusDebug debug;
  debug.compatibilityPolicy.reset(policy);

  DoConnect(p, 3);
  p.SetUserActor(3, 0xff000000);

  debug.Notification(VarValue::AttachTestStackId(),
                     { VarValue("Hello, world!") });
  debug.Notification(VarValue::AttachTestStackId(),
                     { VarValue("Hello, \"world!\"") });

  p.Tick(); // Tick deferred messages

  REQUIRE(p.Messages().size() == 3);
  REQUIRE(p.Messages()[1].userId == 3);
  REQUIRE(p.Messages()[1].reliable);
  REQUIRE(p.Messages()[1].j ==
          nlohmann::json{ { "type", "spSnippet" },
                          { "snippetIdx", 0 },
                          { "selfId", 0 },
                          { "class", "debug" },
                          { "function", "Notification" },
                          { "arguments", { "Hello, world!" } } });
  REQUIRE(p.Messages()[2].userId == 3);
  REQUIRE(p.Messages()[2].reliable);
  REQUIRE(p.Messages()[2].j ==
          nlohmann::json{ { "type", "spSnippet" },
                          { "snippetIdx", 1 },
                          { "selfId", 0 },
                          { "class", "debug" },
                          { "function", "Notification" },
                          { "arguments", { "Hello, \"world!\"" } } });

  DoDisconnect(p, 3);
  p.worldState.DestroyForm(0xff000000);
}
