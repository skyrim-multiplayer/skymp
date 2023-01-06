#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

TEST_CASE("ToString/FromString", "[FormDesc]")
{
  REQUIRE(FormDesc(0xAAA, "").ToString() == "aaa");
  REQUIRE(FormDesc(0xAAA, "Skyrim.esm").ToString() == "aaa:Skyrim.esm");

  auto x = FormDesc::FromString("aaa");
  REQUIRE(x.file == "");
  REQUIRE(x.shortFormId == 0xAAA);

  auto v = FormDesc::FromString("aaa:Skyrim.esm");
  REQUIRE(v.file == "Skyrim.esm");
  REQUIRE(v.shortFormId == 0xAAA);
}

TEST_CASE("ToFormId/FromFormId", "[FormDesc]")
{
  std::vector<std::string> list = { "Skyrim.esm", "Update.esm" };

  REQUIRE(FormDesc::FromFormId(0x01000001, list) ==
          FormDesc(0x1, "Update.esm"));
  REQUIRE(FormDesc::FromFormId(0x00000001, list) ==
          FormDesc(0x1, "Skyrim.esm"));
  REQUIRE(FormDesc::FromFormId(0xff000bbb, list) == FormDesc(0xbbb, ""));

  REQUIRE(FormDesc(0x1, "Update.esm").ToFormId(list) == 0x01000001);
  REQUIRE(FormDesc(0x1, "Skyrim.esm").ToFormId(list) == 0x00000001);
  REQUIRE(FormDesc(0x1, "").ToFormId(list) == 0xff000001);
}
