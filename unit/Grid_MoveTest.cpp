#include "Grid.h"
#include <catch2/catch_all.hpp>

// TODO: Test all methods

TEST_CASE("Move", "[Grid]")
{
  Grid gr;
  gr.Move(0xABCD, 125, -40);
  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(125, -40));
}

TEST_CASE("Move2", "[Grid]")
{
  Grid gr;
  gr.Move(0xABCD, 125, -40);
  gr.Move(0xEBCF, 12, -12);
  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(125, -40));
  REQUIRE(gr.GetPos(0xEBCF) == std::pair<int16_t, int16_t>(12, -12));
}

TEST_CASE("Move3", "[Grid]")
{
  Grid gr;
  gr.Move(0xABCD, 125, -40);
  gr.Move(0xEBCF, 12, -12);
  gr.Move(0x2342, 125, -40);
  gr.Move(0x200F, 12, -12);
  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(125, -40));
  REQUIRE(gr.GetPos(0xEBCF) == std::pair<int16_t, int16_t>(12, -12));
  REQUIRE(gr.GetPos(0x2342) == std::pair<int16_t, int16_t>(125, -40));
  REQUIRE(gr.GetPos(0x200F) == std::pair<int16_t, int16_t>(12, -12));
}

TEST_CASE("Move4", "[Grid]")
{
  Grid gr;
  gr.Move(0xABCD, 644, 232);
  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(644, 232));
  gr.Move(0xABCD, -644, -232);
  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(-644, -232));
}

TEST_CASE("Move5", "[Grid]")
{
  Grid gr;
  gr.Move(0xFF00, 645, 232);
  gr.Move(0xABCD, 644, 232);
  gr.Move(0xFF0F0000, 645, 233);
  gr.Move(0xABCF, 644, 233);

  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(644, 232));
  REQUIRE(gr.GetPos(0xFF00) == std::pair<int16_t, int16_t>(645, 232));
  REQUIRE(gr.GetPos(0xABCF) == std::pair<int16_t, int16_t>(644, 233));
  REQUIRE(gr.GetPos(0xFF0F0000) == std::pair<int16_t, int16_t>(645, 233));
}
