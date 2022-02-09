#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "PacketParser.h"

using Catch::Matchers::Contains;

PartOne& GetPartOne();

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

  IActionListener::RawMessageData msgData;
  msgData.userId = 0;

  REQUIRE_THROWS_WITH(p.GetActionListener().OnConsoleCommand(
                        msgData, "additem", { 0x14, 0x12eb7, 0x108 }),
                      Contains("Not enough permissions to use this command"));

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
