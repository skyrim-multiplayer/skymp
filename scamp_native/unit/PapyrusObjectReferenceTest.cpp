#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "ActionListener.h"
#include "EspmGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusObjectReference.h"

extern espm::Loader l;

namespace {
void CreateMpObjectReference(WorldState& worldState, uint32_t id)
{
  auto refr = std::make_unique<MpObjectReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  worldState.AddForm(std::move(refr), id);
}
void CreateMpObjectReference(PartOne& partOne, uint32_t id)
{
  return CreateMpObjectReference(partOne.worldState, id);
}

auto GetDummyMessageData()
{
  static simdjson::dom::parser parser;
  static uint8_t unparsed[] = { Networking::MinPacketId, '{', '}' };

  IActionListener::RawMessageData data;
  data.userId = 1;
  data.unparsed = unparsed;
  data.parsed = parser.parse(std::string("{}")).value();
  data.unparsedLength = std::size(unparsed);
  return data;
};
}

TEST_CASE("GetItemCount/AddItem", "[Papyrus][ObjectReference]")
{
  PartOne p;

  CreateMpObjectReference(p, 0xff000000);

  EspmGameObject ironSword(l.GetBrowser().LookupById(0x12eb7));
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xff000000);

  auto item = VarValue(&ironSword);

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(0));

  refr.AddItem(0x12eb7, 10);
  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(10));
}

TEST_CASE("RemoveItem", "[Papyrus][ObjectReference]")
{
  PartOne p;

  CreateMpObjectReference(p, 0xff000000);
  CreateMpObjectReference(p, 0xff000001);

  EspmGameObject ironSword(l.GetBrowser().LookupById(0x12eb7));
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(0xff000000);
  auto& refr2 = p.worldState.GetFormAt<MpObjectReference>(0xff000001);

  auto item = VarValue(&ironSword);

  refr.AddItem(0x12eb7, 10);
  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(10));

  PapyrusObjectReference().RemoveItem(refr.ToVarValue(),
                                      { item, VarValue(5), VarValue::None() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(5));

  PapyrusObjectReference().RemoveItem(
    refr.ToVarValue(), { item, VarValue(-1), refr2.ToVarValue() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(0));

  REQUIRE(PapyrusObjectReference().GetItemCount(refr2.ToVarValue(),
                                                { item }) == VarValue(5));
}

TEST_CASE("GetAnimationVariableBool", "[Papyrus][ObjectReference]")
{
  FakeSendTarget tgt;
  PartOne p;
  p.pushedSendTarget = &tgt;

  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &tgt);
  DoConnect(p, 1);
  p.SetUserActor(1, 0xff000000, &tgt);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  REQUIRE(PapyrusObjectReference().GetAnimationVariableBool(
            ac.ToVarValue(), { VarValue("bInJumpState") }) == VarValue(false));

  p.GetActionListener().OnUpdateMovement(GetDummyMessageData(), 0, { 0, 0, 0 },
                                         { 0, 0, 0 }, true, false);

  REQUIRE(PapyrusObjectReference().GetAnimationVariableBool(
            ac.ToVarValue(), { VarValue("bInJumpState") }) == VarValue(true));
}