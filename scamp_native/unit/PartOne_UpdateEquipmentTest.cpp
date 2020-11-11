#include "TestUtils.hpp"

TEST_CASE("UpdateEquipment", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne(&tgt);

  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  DoMessage(partOne, 0, jEquipment);
  tgt = {};

  DoConnect(partOne, 1);
  partOne.CreateActor(0xffABCABC, { 11.f, 22.f, 33.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xffABCABC);

  // createActor should contain equipment
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["equipment"] == jEquipment["data"];
                       }) != tgt.messages.end());
  tgt = {};

  DoMessage(partOne, 0, jEquipment);

  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateEquipment &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1 &&
                           m.j["data"] == jEquipment["data"];
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["t"] == MsgType::UpdateEquipment &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 0 &&
                           m.j["data"] == jEquipment["data"];
                       }) != tgt.messages.end());
}