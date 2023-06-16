#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

namespace {
MpObjectReference& CreateMpObjectReference_(WorldState& worldState,
                                            uint32_t id)
{
  auto refr = std::make_unique<MpObjectReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  worldState.AddForm(std::move(refr), id);
  return worldState.GetFormAt<MpObjectReference>(id);
}
}

TEST_CASE("Disable makes ref invisible", "[ObjectReference]")
{

  PartOne p;

  auto& ref = CreateMpObjectReference_(p.worldState, 0xff000000);
  ref.SetCellOrWorld(FormDesc::Tamriel());

  p.CreateActor(0xff000001, { 0, 0, 0 }, 0, 0x3c);
  DoConnect(p, 0);
  p.SetUserActor(0, 0xff000001);
  DoUpdateMovement(p, 0xff000001, 0);

  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000001);

  REQUIRE(ref.GetListeners() == std::set<MpObjectReference*>{ &ac });
  ref.Disable();
  REQUIRE(ref.GetListeners() == std::set<MpObjectReference*>{});

  ref.SetPos(ref.GetPos());
  ref.SetPos(ref.GetPos() + NiPoint3{ 10000, 10000, 10000 });
  REQUIRE(ref.GetListeners() == std::set<MpObjectReference*>{});
  ref.SetPos(ref.GetPos() - NiPoint3{ 10000, 10000, 10000 });

  ref.Enable();
  REQUIRE(ref.GetListeners() == std::set<MpObjectReference*>{ &ac });
}
