#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "ConsoleCommandMessage.h"
#include "MessageEvent.h"
#include "PacketParser.h"

using Catch::Matchers::ContainsSubstring;

PartOne& GetPartOne();

TEST_CASE("ConsoleCommand packet is parsed", "[ConsoleCommand]")
{
  struct TestData {
    RawMessageData rawMsgData;
    std::string commandName;
    std::vector<std::variant<int64_t, std::string>> args;
  } testData;

  auto& partOne = GetPartOne();
  
  auto connection = partOne.onConsoleCommandMessage.connect([&testData](const MessageEvent<ConsoleCommandMessage>& event) {
    testData.rawMsgData = event.rawMsgData;
    testData.commandName = event.message.data.commandName;
    testData.args = event.message.data.args;
  });

  nlohmann::json j{ { "t", MsgType::ConsoleCommand },
                    { "data",
                      { { "commandName", "additem" },
                        { "args", { 0x14, 0x12eb7, 0x1 } } } } };

  auto msg = MakeMessage(j);

  PacketParser p;
  p.TransformPacketIntoAction(
    122, reinterpret_cast<Networking::PacketData>(msg.data()), msg.size(),
    partOne);

  REQUIRE(testData.args ==
          std::vector<std::variant<int64_t, std::string>>{
            int64_t(0x14), int64_t(0x12eb7), int64_t(0x1) });
  REQUIRE(testData.commandName == "additem");
  REQUIRE(testData.rawMsgData.userId == 122);
}

TEST_CASE("AddItem doesn't execute for non-privilleged users",
          "[ConsoleCommand]")
{
  PartOne& p = GetPartOne();

  DoConnect(p, 0);
  p.CreateActor(0xff000000, { 0, 0, 0 }, 0, 0x3c);
  p.SetUserActor(0, 0xff000000);
  auto& ac = p.worldState.GetFormAt<MpActor>(0xff000000);

  RawMessageData msgData;
  msgData.userId = 0;

  ConsoleCommandMessage msg;
  msg.data.commandName = "additem";
  msg.data.args = { int64_t(0x14), int64_t(0x12eb7), int64_t(0x108) };
  REQUIRE_THROWS_WITH(
    p.onConsoleCommandMessage(MessageEvent<ConsoleCommandMessage>{msgData, msg}),
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

  RawMessageData msgData;
  msgData.userId = 0;

  p.Messages().clear();
  ConsoleCommandMessage msg;
  msg.data.commandName = "additem";
  msg.data.args = { int64_t(0x14), int64_t(0x12eb7), int64_t(0x108) };
  p.onConsoleCommandMessage(MessageEvent<ConsoleCommandMessage>{msgData, msg});

  p.Tick(); // send deferred messages

  nlohmann::json expectedInv{
    { "entries", { { { "baseId", 0x12eb7 }, { "count", 0x108 } } } }
  };
  REQUIRE(p.Messages().size() == 2);
  REQUIRE(
    p.Messages()[0].j ==
    nlohmann::json::parse(
      R"({"inventory":{"entries":[{"baseId":77495,"count":264}]},"t":28})"));
  REQUIRE(
    p.Messages()[1].j ==
    nlohmann::json::parse(
      R"({"arguments":[{"formId":77495,"type":"weapon"},264,false],"class":"SkympHacks","function":"AddItem","selfId":0,"snippetIdx":4294967295,"t":30})"));

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

  RawMessageData msgData;
  msgData.userId = 0;

  p.Messages().clear();
  ConsoleCommandMessage msg2;
  msg2.data.commandName = "placeatme";
  msg2.data.args = { int64_t(0x14), int64_t(EncGiant01) };
  p.onConsoleCommandMessage(MessageEvent<ConsoleCommandMessage>{msgData, msg2});

  auto& refr = p.worldState.GetFormAt<MpActor>(0xff000001);
  REQUIRE(refr.GetBaseId() == EncGiant01);
  REQUIRE(refr.GetPos() == ac.GetPos());
  REQUIRE(refr.GetAngle() == NiPoint3(0, 0, 0));
  REQUIRE(refr.GetCellOrWorld() == ac.GetCellOrWorld());
  p.worldState.DestroyForm(0xff000001);

  p.DestroyActor(0xff000000);
  DoDisconnect(p, 0);
}
