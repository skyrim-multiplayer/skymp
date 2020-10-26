#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "PapyrusDebug.h"

TEST_CASE("Notification", "[Papyrus][Debug]")
{
  FakeSendTarget tgt;
  PartOne p;
  {
    auto ac =
      std::make_unique<MpActor>(LocationalData(), p.CreateFormCallbacks(&tgt));
    p.worldState.AddForm(std::move(ac), 0xff000000);
  }

  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  PapyrusDebug debug;
  debug.compatibilityPolicy.reset(new PapyrusCompatibilityPolicy(&ac));

  DoConnect(p, 3);
  p.SetUserActor(3, 0xff000000, &tgt);

  debug.Notification(VarValue::None(), { VarValue("Hello, world!") });
  debug.Notification(VarValue::None(), { VarValue("Hello, \"world!\"") });

  REQUIRE(tgt.messages.size() == 3);
  REQUIRE(tgt.messages[1].userId == 3);
  REQUIRE(tgt.messages[1].reliable);
  REQUIRE(tgt.messages[1].j ==
          nlohmann::json{ { "type", "spSnippet" },
                          { "snippetIdx", 0 },
                          { "selfId", 0 },
                          { "class", "debug" },
                          { "function", "Notification" },
                          { "arguments", { "Hello, world!" } } });
  REQUIRE(tgt.messages[2].userId == 3);
  REQUIRE(tgt.messages[2].reliable);
  REQUIRE(tgt.messages[2].j ==
          nlohmann::json{ { "type", "spSnippet" },
                          { "snippetIdx", 1 },
                          { "selfId", 0 },
                          { "class", "debug" },
                          { "function", "Notification" },
                          { "arguments", { "Hello, \"world!\"" } } });

  DoDisconnect(p, 3);
  p.worldState.DestroyForm(0xff000000);
}