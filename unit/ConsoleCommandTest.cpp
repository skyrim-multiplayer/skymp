#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "PacketParser.h"

using Catch::Matchers::ContainsSubstring;

PartOne& GetPartOne();

TEST_CASE("ConsoleCommand packet is parsed", "[ConsoleCommand]")
{
  class MyActionListener : public ActionListener
  {
  public:
    MyActionListener()
      : ActionListener(GetPartOne())
    {
    }

    void OnConsoleCommand(
      const RawMessageData& rawMsgData_,
      const std::string& consoleCommandName_,
      const std::vector<ConsoleCommands::Argument>& args_) override
    {
      rawMsgData = rawMsgData_;
      consoleCommandName = consoleCommandName_;
      args = args_;
    }

    RawMessageData rawMsgData;
    std::string consoleCommandName;
    std::vector<ConsoleCommands::Argument> args;
  };

  nlohmann::json j{ { "t", MsgType::ConsoleCommand },
                    { "data",
                      { { "commandName", "additem" },
                        { "args", { 0x14, 0x12eb7, 0x1 } } } } };

  auto msg = MakeMessage(j);

  MyActionListener listener;

  PacketParser p;
  p.TransformPacketIntoAction(
    122, reinterpret_cast<Networking::PacketData>(msg.data()), msg.size(),
    listener);

  REQUIRE(listener.args ==
          std::vector<ConsoleCommands::Argument>({ 0x14, 0x12eb7, 0x1 }));
  REQUIRE(listener.consoleCommandName == "additem");
  REQUIRE(listener.rawMsgData.userId == 122);
}

TEST_CASE("AddItem doesn't execute for non-privilleged users",
          "[ConsoleCommand]")
{
  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  REQUIRE_THROWS_WITH(
    p.GetActionListener().OnConsoleCommand(msgData, "additem",
                                           { 0x14, 0x12eb7, 0x108 }),
    ContainsSubstring("Not enough permissions to use this command"));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("AddItem executes", "[ConsoleCommand][espm]")
{
  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.SetConsoleCommandsAllowedFlag(true);
  ac.RemoveAllItems();

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  p.Messages().clear();
  p.GetActionListener().OnConsoleCommand(msgData, "additem",
                                         { 0x14, 0x12eb7, 0x108 });

  nlohmann::json expectedInv{
    { "entries", { { { "baseId", 0x12eb7 }, { "count", 0x108 } } } }
  };
  REQUIRE(p.Messages().size() == 2);
  REQUIRE(
    p.Messages()[0].j.dump() ==
    R"({"inventory":{"entries":[{"baseId":77495,"count":264}]},"type":"setInventory"})");
  REQUIRE(
    p.Messages()[1].j.dump() ==
    R"({"arguments":[{"formId":77495,"type":"weapon"},264,false],"class":"SkympHacks","function":"AddItem","selfId":0,"snippetIdx":0,"type":"spSnippet"})");

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}

TEST_CASE("PlaceAtMe executes", "[ConsoleCommand][espm]")
{
  enum
  {
    EncGiant01 = 0x00023aae
  };

  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.SetConsoleCommandsAllowedFlag(true);

  ActionListener::RawMessageData msgData;
  msgData.userId = 0;

  p.Messages().clear();
  p.GetActionListener().OnConsoleCommand(msgData, "placeatme",
                                         { 0x14, EncGiant01 });

  auto& refr = p.worldState.GetFormAt<MpActor>(0xff000001);
  REQUIRE(refr.GetBaseId() == EncGiant01);
  REQUIRE(refr.GetPos() == ac.GetPos());
  REQUIRE(refr.GetAngle() == NiPoint3(0, 0, 0));
  REQUIRE(refr.GetCellOrWorld() == ac.GetCellOrWorld());
  p.worldState.DestroyForm(0xff000001);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
