#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "FindRecipe.h"
#include "PacketParser.h"

using Catch::Matchers::ContainsSubstring;

PartOne& GetPartOne();

TEST_CASE("CraftItem packet is parsed", "[Craft][espm]")
{
  class MyActionListener : public ActionListener
  {
  public:
    MyActionListener()
      : ActionListener(GetPartOne())
    {
    }

    void OnCraftItem(const RawMessageData& rawMsgData_,
                     const Inventory& inputObjects_, uint32_t workbenchId_,
                     uint32_t resultObjectId_) override
    {
      rawMsgData = rawMsgData_;
      inputObjects = inputObjects_;
      workbenchId = workbenchId_;
      resultObjectId = resultObjectId_;
    }

    RawMessageData rawMsgData;
    Inventory inputObjects;
    uint32_t workbenchId = 0;
    uint32_t resultObjectId = 0;
  };

  nlohmann::json j{
    { "t", MsgType::CraftItem },
    { "data",
      { { "workbench", 0xdeadbeef },
        { "resultObjectId", 0x123 },
        { "craftInputObjects", Inventory().AddItem(0x12eb7, 1).ToJson() } } }
  };

  auto msg = MakeMessage(j);

  MyActionListener listener;

  PacketParser p;
  p.TransformPacketIntoAction(
    122, reinterpret_cast<Networking::PacketData>(msg.data()), msg.size(),
    listener);

  REQUIRE(listener.workbenchId == 0xdeadbeef);
  REQUIRE(listener.resultObjectId == 0x123);
  REQUIRE(listener.inputObjects == Inventory().AddItem(0x12eb7, 1));
  REQUIRE(listener.rawMsgData.userId == 122);
}

TEST_CASE("Player is able to craft item", "[Craft][espm]")
{
  const Inventory requiredItems =
    Inventory().AddItem(0x5ace4, 1).AddItem(0x800e4, 3).AddItem(0x5ace5, 4);
  const Inventory requiredItemsForNails = Inventory().AddItem(0x5ace4, 1);

  PartOne& p = GetPartOne();

  const auto workbenchId = 0x1ad6e;
  auto& refr = p.worldState.GetFormAt<MpObjectReference>(workbenchId);

  DoConnect(p, 0);
  p.CreateActor(0xff000000, refr.GetPos(), 0,
                refr.GetCellOrWorld().ToFormId(p.worldState.espmFiles));
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  for (auto entry : requiredItems.entries)
    ac.AddItem(entry.baseId, entry.count);
  for (auto entry : requiredItemsForNails.entries)
    ac.AddItem(entry.baseId, entry.count);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  // Vanilla item
  REQUIRE(ac.GetInventory().GetItemCount(0x1398a) == 0);
  p.GetActionListener().OnCraftItem(msgData, requiredItems, workbenchId,
                                    0x1398a);
  REQUIRE(ac.GetInventory().GetItemCount(0x1398a) == 1);

  // Hearthfires item (nails)
  REQUIRE(ac.GetInventory().GetItemCount(0x300300f) == 0);
  p.GetActionListener().OnCraftItem(msgData, requiredItemsForNails,
                                    workbenchId, 0x300300f);
  REQUIRE(ac.GetInventory().GetItemCount(0x300300f) == 10);

  REQUIRE(ac.GetInventory().GetItemCount(0x5ace4) == 0);
  REQUIRE(ac.GetInventory().GetItemCount(0x800e4) == 0);
  REQUIRE(ac.GetInventory().GetItemCount(0x5ace5) == 0);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE(
  "Player is unable to craft an artifact item by using a tempering recipe",
  "[Craft][espm]")
{
  auto DeerPelt = 0x000CF89E;
  auto LeatherStrips = 0x000800E4;
  auto WolfPelt = 0x0003AD74;
  auto Leather = 0x000DB5D2;
  const Inventory requiredItems =
    Inventory()
      .AddItem(DeerPelt, 1)
      .AddItem(LeatherStrips, 2)
      .AddItem(WolfPelt, 1)
      .AddItem(Leather, 1); // Required for temper in vanila

  PartOne& p = GetPartOne();
  const auto workbenchId = 0x1ad6e;
  auto& workbench = p.worldState.GetFormAt<MpObjectReference>(workbenchId);

  DoConnect(p, 0);
  p.CreateActor(0xff000000, workbench.GetPos(), 0,
                workbench.GetCellOrWorld().ToFormId(p.worldState.espmFiles));
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  for (auto entry : requiredItems.entries)
    ac.AddItem(entry.baseId, entry.count);

  const uint32_t wrongResultObject = 0xd8d4e;

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  REQUIRE_THROWS_WITH(p.GetActionListener().OnCraftItem(msgData, requiredItems,
                                                        workbenchId,
                                                        wrongResultObject),
                      ContainsSubstring("Recipe not found"));
}

TEST_CASE("DLC Dragonborn recipes are working", "[Craft][espm]")
{

  PartOne& p = GetPartOne();
  auto form = FindRecipe(p.GetEspm().GetBrowser(),
                         Inventory()
                           .AddItem(0x0005ACE4, 1)
                           .AddItem(0x0401CD7C, 2)
                           .AddItem(0x00034CDD, 10),
                         0x04037564);
  REQUIRE(form);
  REQUIRE(form->GetId() == 0x0203d581);
}

TEST_CASE("DLC Hearthfires recipes are working", "[Craft][espm]")
{
  PartOne& p = GetPartOne();

  REQUIRE(RecipeMatches(p.GetEspm().GetBrowser().GetCombMapping(3),
                        espm::Convert<espm::COBJ>(
                          p.GetEspm().GetBrowser().LookupById(0x0300306d).rec),
                        Inventory().AddItem(0x0005ACE4, 1),
                        0x300300F) == true);

  auto form = FindRecipe(p.GetEspm().GetBrowser(),
                         Inventory().AddItem(0x0005ACE4, 1), 0x300300F);
  REQUIRE(form);
  REQUIRE(form->GetId() == 0x0200306d);
}
