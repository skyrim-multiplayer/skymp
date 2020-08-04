#include "TestUtils.hpp"

TEST_CASE("Hypothesis: UpdateMovement may send nothing when actor without "
          "user present",
          "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  constexpr uint32_t n = 20;
  static_assert(n <= MAX_PLAYERS - 1);

  for (uint32_t i = 0; i < n; ++i) {
    partOne.CreateActor(i + 0xff000000, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
    if (i % 2 == 0)
      continue;

    DoConnect(partOne, i + 1);
    partOne.SetUserActor(i + 1, i + 0xff000000, &tgt);

    DoUpdateMovement(partOne, i + 0xff000000, i + 1);
  }

  DoConnect(partOne, 0);
  partOne.CreateActor(0xffffffff, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xffffffff, &tgt);
  tgt = {};

  DoUpdateMovement(partOne, 0xffffffff, 0);
  REQUIRE(tgt.messages.size() == 11); // Me and 10 other users created in loop
}

TEST_CASE("UpdateMovement", "[PartOne]")
{
  FakeSendTarget tgt;
  PartOne partOne;
  partOne.pushedSendTarget = &tgt;

  auto doMovement = [&] { DoMessage(partOne, 0, jMovement); };

  DoConnect(partOne, 0);

  doMovement();
  REQUIRE(tgt.messages.size() == 0); // No actor - no movement

  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  partOne.SetUserActor(0, 0xff000ABC, &tgt);
  tgt = {};
  doMovement();
  REQUIRE(tgt.messages.size() == 1);
  REQUIRE(tgt.messages.at(0).j.dump() == jMovement.dump());

  // UpdateMovement actually changes position and rotation
  REQUIRE(
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormById(0xff000ABC).get())
      ->GetPos() == NiPoint3{ 1, -1, 1 });
  REQUIRE(
    dynamic_cast<MpActor*>(partOne.worldState.LookupFormById(0xff000ABC).get())
      ->GetAngle() == NiPoint3{ 0, 0, 179 });

  // Another player connects and see us
  DoConnect(partOne, 1);
  partOne.CreateActor(0xff00ABCD, { 1.f, 2.f, 3.f }, 180.f, 0x3c, &tgt);
  tgt = {};
  partOne.SetUserActor(1, 0xff00ABCD, &tgt);
  REQUIRE(tgt.messages.size() == 3);
  // Create idx 1 for user 0, then idx 0 for 1, then idx 1 for 1 (self
  // streaming)
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 0;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "createActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());

  // Look must be empty by default
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["look"] != nullptr;
                       }) == tgt.messages.end());

  tgt = {};
  doMovement();
  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(tgt.messages.at(0).j.dump() == jMovement.dump());
  REQUIRE(tgt.messages.at(1).j.dump() == jMovement.dump());

  // Another player is being moved away and now doesn't see our movement
  auto acAbcd = dynamic_cast<MpActor*>(
    partOne.worldState.LookupFormById(0xff00ABCD).get());
  tgt = {};
  acAbcd->SetPos({ 100000, 0, 0 });
  REQUIRE(tgt.messages.size() == 2);
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "destroyActor" &&
                           m.j["idx"] == 1 && m.reliable && m.userId == 0;
                       }) != tgt.messages.end());
  REQUIRE(std::find_if(tgt.messages.begin(), tgt.messages.end(),
                       [&](FakeSendTarget::Message m) {
                         return m.j["type"] == "destroyActor" &&
                           m.j["idx"] == 0 && m.reliable && m.userId == 1;
                       }) != tgt.messages.end());
}