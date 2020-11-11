#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "PacketParser.h"

PartOne& GetPartOne();
extern FakeSendTarget g_tgt;

TEST_CASE("ConsoleCommand packet is parsed", "[ConsoleCommand]")
{
  class MyActionListener : public IActionListener
  {
  public:
    void OnConsoleCommand(
      const RawMessageData& rawMsgData_,
      const std::string& consoleCommandName_,
      const std::vector<ConsoleCommands::Argument>& args_) override
    {
      rawMsgData = &rawMsgData_;
      consoleCommandName = consoleCommandName_;
      args = args_;
    }

    const RawMessageData* rawMsgData = nullptr;
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
  REQUIRE(listener.rawMsgData->userId == 122);
}

/*TEST_CASE("AddItem executes", "[ConsoleCommand]")
{
  PartOne& p = GetPartOne();
  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c, &g_tgt);
  p.SetUserActor(0, 0xff000000, &g_tgt);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);
  ac.RegisterProfileId(MpActor::ProfileIds::kProfileId_Pospelov);

  g_tgt = {};

  IActionListener::RawMessageData msgData;
  msgData.userId = 0;

  
  p.pushedSendTarget = nullptr;
  p.GetActionListener().OnConsoleCommand(msgData, "additem",
                                         { 0x14, 0x12eb7, 0x108 });

  nlohmann::json expectedInv{
    { "entries", { { { "baseId", 0x12eb7 }, { "count", 0x108 } } } }
  };
  REQUIRE(g_tgt.messages.size() == 1);
  REQUIRE(g_tgt.messages[0].j["type"] == "setInventory");
  REQUIRE(g_tgt.messages[0].j["inventory"].dump() == expectedInv.dump());

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}*/