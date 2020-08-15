#include "TestUtils.hpp"

TEST_CASE("Activate", "[PartOne]")
{
  auto lst = FakeListener::New();
  PartOne partOne(lst);

  DoConnect(partOne, 0);
  REQUIRE_THROWS_WITH(
    DoMessage(partOne, 0,
              nlohmann::json{
                { "t", MsgType::Activate },
                { "data", { { "caster", 0x14 }, { "target", 0xbeb } } } }),
    Contains("beb is not a valid activation target"));

  // REQUIRE_THAT(lst->str(), Contains("OnCustomPacket(0, {\"x\":\"y\"})"));
}