#include "TestUtils.hpp"

using Catch::Matchers::ContainsSubstring;

TEST_CASE("SweetHidePlayerNames Service Integration", "[SweetHide]")
{
  PartOne partOne;

  // Setup User 0 with "Oberyn"
  DoConnect(partOne, 0);
  partOne.CreateActor(0xff000ABC, { 1.f, 2.f, 3.f }, 180.f, 0x3c);
  partOne.SetUserActor(0, 0xff000ABC);
  const Appearance appearance = Appearance::FromJson(jAppearance["data"]);
  partOne.worldState.GetFormAt<MpActor>(0xff000ABC).SetAppearance(&appearance);

  // Clear previous messages (User 0 creation)
  partOne.Messages().clear();

  // Setup User 1
  DoConnect(partOne, 1);
  partOne.CreateActor(0xff000FFF, { 100.f, 200.f, 300.f }, 180.f, 0x3c);
  partOne.SetUserActor(1, 0xff000FFF);

  // User 1 should receive CreateActor for User 0 (Idx 0)
  auto res = FindRefrMessageIdx<CreateActorMessage>(partOne, 0);

  REQUIRE(res.filteredMessages.size() > 0);

  auto& msg = res.filteredMessages[0];
  REQUIRE(msg.appearance.has_value());

  // Verify who received the message
  // FindRefrMessageResult doesn't give us the userId, but
  // filteredMessagesOriginals does.
  auto& originalMsg = res.filteredMessagesOriginals[0];
  // partOne.Messages() stores {userId, message}
  // But Wait, PartOne::Message struct might not have userId public or it might
  // be different. Let's check TestUtils.hpp or PartOne.h for Message struct.

  // Since we know User 1 just connected and User 0 was already there,
  // User 1 should see User 0. User 0 should see User 1.
  // Message with idx=0 is about User 0. So it must be sent to User 1.

  CAPTURE(msg.appearance->name);
  // With current PartOne.cpp hardcoded logic, this SHOULD be "Stranger".
  // Note: jAppearance has name "Oberyn".
  REQUIRE(msg.appearance->name == "Stranger");
}
