#include "TestUtils.hpp"
#include <catch2/catch.hpp>

namespace {
MpObjectReference& CreateMpObjectReference(WorldState& worldState, uint32_t id)
{
  auto refr = std::make_unique<MpObjectReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  worldState.AddForm(std::move(refr), id);
  return worldState.GetFormAt<MpObjectReference>(id);
}
}

TEST_CASE("Disable makes ref invisible", "[ObjectReference]")
{
  FakeSendTarget tgt;
  PartOne p;
  p.pushedSendTarget = &tgt;

  auto& ref = CreateMpObjectReference(p.worldState, 0xff000000);
  ref.SetCellOrWorld(0x3c);

  p.CreateActor(0xff000001, { 0, 0, 0 }, 0, 0x3c, &tgt);
  DoConnect(p, 0);
  p.SetUserActor(0, 0xff000001, &tgt);
  DoUpdateMovement(p, 0xff000001, 0);

  // p.worldState.GetFormAt<MpActor>(0xff000001).SetPos(1);

  REQUIRE(ref.GetListeners().size() == -1);
  // ref.Disable();
  // REQUIRE(ref.GetListeners().size() == 0);
}

TEST_CASE("Enable makes ref visible", "[ObjectReference]")
{
}