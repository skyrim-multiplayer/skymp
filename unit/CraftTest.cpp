#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "CraftItemMessage.h"
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
                     const CraftItemMessage& msg_) override
    {
      rawMsgData = rawMsgData_;
      inputObjects = msg_.data.craftInputObjects;
      workbenchId = msg_.data.workbench;
      resultObjectId = msg_.data.resultObjectId;
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

  RawMessageData msgData;
  msgData.userId = 0;

  // Vanilla item
  REQUIRE(ac.GetInventory().GetItemCount(0x1398a) == 0);
  CraftItemMessage msg1;
  msg1.data.craftInputObjects = requiredItems;
  msg1.data.workbench = workbenchId;
  msg1.data.resultObjectId = 0x1398a;
  p.GetActionListener().OnCraftItem(msgData, msg1);
  REQUIRE(ac.GetInventory().GetItemCount(0x1398a) == 1);

  // Hearthfires item (nails)
  REQUIRE(ac.GetInventory().GetItemCount(0x300300f) == 0);
  CraftItemMessage msg2;
  msg2.data.craftInputObjects = requiredItemsForNails;
  msg2.data.workbench = workbenchId;
  msg2.data.resultObjectId = 0x300300f;
  p.GetActionListener().OnCraftItem(msgData, msg2);
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

  RawMessageData msgData;
  msgData.userId = 0;

  Inventory previousInventory = ac.GetInventory();

  // Must result in "Recipe not found" in logs
  CraftItemMessage msg3;
  msg3.data.craftInputObjects = requiredItems;
  msg3.data.workbench = workbenchId;
  msg3.data.resultObjectId = wrongResultObject;
  p.GetActionListener().OnCraftItem(msgData, msg3);

  Inventory newInventory = ac.GetInventory();

  REQUIRE(previousInventory == newInventory);
}

TEST_CASE("DLC Dragonborn recipes are working", "[Craft][espm]")
{

  PartOne& p = GetPartOne();
  auto craftService = p.GetActionListener().GetCraftService();

  auto form = craftService->FindRecipe(std::nullopt, std::nullopt,
                                       p.GetEspm().GetBrowser(),
                                       Inventory()
                                         .AddItem(0x0005ACE4, 1)
                                         .AddItem(0x0401CD7C, 2)
                                         .AddItem(0x00034CDD, 10),
                                       0x04037564);
  REQUIRE(form.size() == 1);
  REQUIRE(form[0].rec->GetId() == 0x0203d581);
}

TEST_CASE("DLC Hearthfires recipes are working", "[Craft][espm]")
{
  PartOne& p = GetPartOne();
  auto craftService = p.GetActionListener().GetCraftService();

  auto recipe = p.GetEspm().GetBrowser().LookupById(0x0300306d);
  auto inputObjects = Inventory().AddItem(0x0005ACE4, 1);
  bool matches =
    craftService->RecipeItemsMatch(recipe, inputObjects, 0x300300F);

  REQUIRE(matches == true);

  auto form = craftService->FindRecipe(
    std::nullopt, std::nullopt, p.GetEspm().GetBrowser(),
    Inventory().AddItem(0x0005ACE4, 1), 0x300300F);
  REQUIRE(form.size() > 0);
  REQUIRE(form[0].rec->GetId() == 0x0200306d);
}
