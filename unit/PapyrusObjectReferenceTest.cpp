#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "ActionListener.h"
#include "MpObjectReference.h"
#include "papyrus-vm/Structures.h"
#include "script_classes/PapyrusObjectReference.h"
#include "script_objects/EspmGameObject.h"

using Catch::Matchers::ContainsSubstring;

extern espm::Loader l;

namespace {

class TestReference : public MpObjectReference
{
public:
  TestReference(const LocationalData& locationalData,
                const FormCallbacks& callbacks, uint32_t baseId,
                std::string baseType,
                std::optional<NiPoint3> primitiveBoundsDiv2 = std::nullopt)
    : MpObjectReference(locationalData, callbacks, baseId, baseType,
                        primitiveBoundsDiv2)
  {
  }

  void SendPapyrusEvent(const char* eventName,
                        const VarValue* arguments = nullptr,
                        size_t argumentsCount = 0) override
  {
    events.push_back(eventName);
    return MpObjectReference::SendPapyrusEvent(eventName, arguments,
                                               argumentsCount);
  }

  std::vector<std::string> events;
};

TestReference& CreateMpObjectReference(WorldState& worldState, uint32_t id)
{
  auto refr = std::make_unique<TestReference>(
    LocationalData(), FormCallbacks::DoNothing(), 0, "CONT");
  worldState.AddForm(std::move(refr), id);
  return worldState.GetFormAt<TestReference>(id);
}
TestReference& CreateMpObjectReference(PartOne& partOne, uint32_t id)
{
  return CreateMpObjectReference(partOne.worldState, id);
}

auto GetDummyMessageData()
{
  static simdjson::dom::parser parser;
  static uint8_t unparsed[] = { Networking::MinPacketId, '{', '}' };

  ActionListener::RawMessageData data;
  data.userId = 1;
  data.unparsed = unparsed;
  data.parsed = parser.parse(std::string("{}")).value();
  data.unparsedLength = std::size(unparsed);
  return data;
};
}

TEST_CASE("GetItemCount/AddItem", "[Papyrus][ObjectReference][espm]")
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

TEST_CASE("RemoveItem", "[Papyrus][ObjectReference][espm]")
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

  PapyrusObjectReference().RemoveItem(
    refr.ToVarValue(),
    { item, VarValue(5), VarValue(true), VarValue::None() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(5));

  PapyrusObjectReference().RemoveItem(
    refr.ToVarValue(),
    { item, VarValue(-1), VarValue(true), refr2.ToVarValue() });

  REQUIRE(PapyrusObjectReference().GetItemCount(refr.ToVarValue(), { item }) ==
          VarValue(0));

  REQUIRE(PapyrusObjectReference().GetItemCount(refr2.ToVarValue(),
                                                { item }) == VarValue(5));
}

TEST_CASE("GetAnimationVariableBool", "[Papyrus][ObjectReference][espm]")
{

  PartOne p;

  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  DoConnect(p, 1);
  p.SetUserActor(1, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  REQUIRE(PapyrusObjectReference().GetAnimationVariableBool(
            ac.ToVarValue(), { VarValue("bInJumpState") }) == VarValue(false));

  p.GetActionListener().OnUpdateMovement(GetDummyMessageData(), 0, { 0, 0, 0 },
                                         { 0, 0, 0 }, true, false, false, 0x3c,
                                         "Standing");

  REQUIRE(PapyrusObjectReference().GetAnimationVariableBool(
            ac.ToVarValue(), { VarValue("bInJumpState") }) == VarValue(true));
}

TEST_CASE("BlockActivation", "[Papyrus][ObjectReference][espm]")
{
  PartOne p;
  p.CreateActor(0xff000001, { 0, 0, 0 }, 0, 0x3c);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000001);

  auto& refr = CreateMpObjectReference(p, 0xff000000);

  // Trying to perform activation and fails due to unattached espm
  REQUIRE_THROWS_WITH(refr.Activate(ac),
                      ContainsSubstring("No espm attached"));

  PapyrusObjectReference().BlockActivation(refr.ToVarValue(),
                                           { VarValue(true) });

  refr.events.clear();
  // Nothing happens, but event is fired
  refr.Activate(ac);
  REQUIRE(refr.events.size() == 1);
  REQUIRE(refr.events[0] == "OnActivate");

  PapyrusObjectReference().BlockActivation(refr.ToVarValue(),
                                           { VarValue(false) });
  REQUIRE_THROWS_WITH(refr.Activate(ac),
                      ContainsSubstring("No espm attached"));
}

TEST_CASE("MoveTo", "[Papyrus][ObjectReference]")
{
  PapyrusObjectReference papyrusObjectReference;
  PartOne partOne;
  DoConnect(partOne, 0);
  uint32_t formId =
    partOne.CreateActor(0xff000000, { 666, 666, 666 }, 0, 0x3c);
  partOne.SetUserActor(0, 0xff000000);
  auto& messages = partOne.Messages();
  auto& actor = partOne.worldState.GetFormAt<MpActor>(formId);
  auto& refr = CreateMpObjectReference(partOne, 0xff000001);
  REQUIRE(actor.GetPos() != refr.GetPos());
  papyrusObjectReference.MoveTo(refr.ToVarValue(),
                                { actor.ToVarValue(), VarValue(0), VarValue(0),
                                  VarValue(0), VarValue(true) });
  REQUIRE(refr.GetPos() == actor.GetPos());
  REQUIRE(refr.GetCellOrWorld() == actor.GetCellOrWorld());
  refr.SetPos({ 0, 0, 0 });
  papyrusObjectReference.MoveTo(actor.ToVarValue(),
                                { refr.ToVarValue(), VarValue(0), VarValue(0),
                                  VarValue(0), VarValue(true) });
  REQUIRE(refr.GetPos() == actor.GetPos());
  REQUIRE(refr.GetCellOrWorld() == actor.GetCellOrWorld());
  {
    auto it = std::find_if(
      messages.begin(), messages.end(),
      [](PartOne::Message& msg) { return msg.j["t"] == MsgType::Teleport; });
    REQUIRE(it != messages.end());
  }
}
