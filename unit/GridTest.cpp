// Copied from
// https://gitlab.com/pospelov/skymp2-server/-/raw/master/src/tests/GridTest.cpp
#include "Grid.h"
#include <catch2/catch_all.hpp>

using formid = uint64_t;

TEST_CASE("Forget", "[Grid]")
{
  Grid gr;
  gr.Move(0xABCD, -111, -111);
  gr.Forget(0xABCD);
  try {
    gr.GetPos(0xABCD);
  } catch (std::exception& e) {
    REQUIRE(1);
    return;
  }
  REQUIRE(0);
}

TEST_CASE("GetNeighbours", "[Grid]")
{
  Grid gr;
  gr.Move(0xFF00, 645, 232);
  gr.Move(0xABCD, 644, 232);
  gr.Move(0xFF0F0000, 645, 233);
  gr.Move(0xABCF, 644, 233);
  REQUIRE(gr.GetNeighbours(0xABCF) ==
          std::set<formid>({ 0xABCD, 0xFF00, 0xFF0F0000 }));

  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(644, 232));
  REQUIRE(gr.GetPos(0xFF00) == std::pair<int16_t, int16_t>(645, 232));
  REQUIRE(gr.GetPos(0xABCF) == std::pair<int16_t, int16_t>(644, 233));
  REQUIRE(gr.GetPos(0xFF0F0000) == std::pair<int16_t, int16_t>(645, 233));
}

TEST_CASE("GetNeighbours2 + Forget", "[Grid]")
{
  Grid gr;
  gr.Move(0xFF00, 645, 232);
  gr.Move(0xABCD, 644, 232);
  gr.Move(0xFF0F0000, 645, 233);
  gr.Move(0xABCF, 644, 233);
  gr.Forget(0xFF00);
  REQUIRE(gr.GetNeighbours(0xABCF) ==
          std::set<formid>({ 0xABCD, 0xFF0F0000 }));

  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(644, 232));
  REQUIRE(gr.GetPos(0xABCF) == std::pair<int16_t, int16_t>(644, 233));
  REQUIRE(gr.GetPos(0xFF0F0000) == std::pair<int16_t, int16_t>(645, 233));
}

TEST_CASE("GetNeighbours3 + Forget", "[Grid]")
{
  Grid gr;
  gr.Move(0xFF00, 645, 232);
  gr.Move(0xABCD, 644, 232);
  gr.Move(0xFF0F0000, 645, 233);
  gr.Move(0xABCF, 644, 233);
  REQUIRE(gr.GetNeighbours(0xABCD) ==
          std::set<formid>({ 0xABCF, 0xFF00, 0xFF0F0000 }));
  gr.Forget(0xFF00);
  gr.Forget(0xFF0F0000);
  REQUIRE(gr.GetNeighbours(0xABCF) == std::set<formid>({ 0xABCD }));
  gr.Move(0xABCD, 644, 300);
  REQUIRE(gr.GetNeighbours(0xABCF) == std::set<formid>({}));
  gr.Move(0xFF0F0000, 645, 233);
  REQUIRE(gr.GetNeighbours(0xABCF) == std::set<formid>({ 0xFF0F0000 }));

  REQUIRE(gr.GetPos(0xABCD) == std::pair<int16_t, int16_t>(644, 300));
  REQUIRE(gr.GetPos(0xABCF) == std::pair<int16_t, int16_t>(644, 233));
  REQUIRE(gr.GetPos(0xFF0F0000) == std::pair<int16_t, int16_t>(645, 233));
}

TEST_CASE("GetNeighbours4 + Forget", "[Grid]")
{
  Grid gr;
  gr.Move(0xA200, 0, 0);
  gr.Move(0xA111, -1, 0);
  gr.Move(0xA101, 0, -1);
  gr.Move(0xA100, -1, -1);

  REQUIRE(gr.GetNeighbours(0xA200) ==
          std::set<formid>({ 0xA100, 0xA101, 0xA111 }));
  REQUIRE(gr.GetNeighbours(0xA111) ==
          std::set<formid>({ 0xA100, 0xA101, 0xA200 }));
  REQUIRE(gr.GetNeighbours(0xA101) ==
          std::set<formid>({ 0xA100, 0xA111, 0xA200 }));
  REQUIRE(gr.GetNeighbours(0xA100) ==
          std::set<formid>({ 0xA101, 0xA111, 0xA200 }));

  gr.Move(0xA201, 0, 0);

  REQUIRE(gr.GetNeighbours(0xA200) ==
          std::set<formid>({ 0xA100, 0xA101, 0xA111, 0xA201 }));

  gr.Forget(0xA100);
  gr.Forget(0xA101);
  gr.Forget(0xA111);
  gr.Forget(0xA200);

  REQUIRE(gr.GetNeighbours(0xA201) == std::set<formid>({}));

  REQUIRE(gr.GetPos(0xA201) == std::pair<int16_t, int16_t>(0, 0));
}

TEST_CASE("GetNeighbours5 + Forget", "[Grid]")
{
  Grid gr;
  gr.Move(0xA001, 0, 0);
  gr.Move(0xA002, 1, -1);
  REQUIRE(gr.GetNeighbours(0xA001) == std::set<formid>({ 0xA002 }));
  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({ 0xA001 }));

  gr.Move(0xA002, 10, 10);

  REQUIRE(gr.GetNeighbours(0xA001) == std::set<formid>({}));
  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({}));

  gr.Move(0xA001, 10, 10);

  REQUIRE(gr.GetNeighbours(0xA001) == std::set<formid>({ 0xA002 }));
  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({ 0xA001 }));

  gr.Move(0xA001, 100, 10);

  REQUIRE(gr.GetNeighbours(0xA001) == std::set<formid>({}));
  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({}));

  gr.Move(0xA002, 101, 9);

  REQUIRE(gr.GetNeighbours(0xA001) == std::set<formid>({ 0xA002 }));
  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({ 0xA001 }));

  gr.Forget(0xA001);

  REQUIRE(gr.GetNeighbours(0xA002) == std::set<formid>({}));
  REQUIRE(gr.GetPos(0xA002) == std::pair<int16_t, int16_t>(101, 9));
}

TEST_CASE("MoveWithDiff", "[Grid]")
{
  Grid gr;

  // When first moved, create an object and self-add
  auto diff0 = gr.MoveWithDiff(0xA001, 0, 0);
  REQUIRE(diff0.added == std::set<formid>({ 0xA001 }));

  // Nothing changed
  auto diff1 = gr.MoveWithDiff(0xA001, 0, 0);
  REQUIRE(diff1.added == std::set<formid>({}));

  // 0xA002: create an object and self-add
  // 0xA001: already exists
  auto diff2 = gr.MoveWithDiff(0xA002, 0, 0);
  REQUIRE(diff2.added == std::set<formid>({ 0xA001, 0xA002 }));
}

TEST_CASE("MoveWithDiff - Initial spawn at (0,0) activation bug", "[Grid]")
{
  Grid gr;

  // 1. Spawn Object 1 at a non-zero coordinate.
  // This bypasses the bug because default (0,0) != (1,1).
  auto diff1 = gr.MoveWithDiff(0xA001, 1, 1);
  REQUIRE(diff1.added == std::set<formid>({ 0xA001 }));
  REQUIRE(gr.GetPos(0xA001) == std::pair<int16_t, int16_t>(1, 1));

  // 2. Spawn Object 2 EXACTLY at (0, 0).
  // If the bug exists, diff2.added will be empty because the default
  // struct coords of (0,0) bypass the "if (coords != target)" check.
  auto diff2 = gr.MoveWithDiff(0xA002, 0, 0);
  REQUIRE(diff2.added == std::set<formid>({ 0xA002 }));

  // Verify the grid actually considers the object "active".
  // If the bug exists, GetPos might throw an exception or return garbage
  // data depending on how it handles inactive objects.
  REQUIRE(gr.GetPos(0xA002) == std::pair<int16_t, int16_t>(0, 0));

  // 3. Spawn Object 3 at (0, 0).
  // It should correctly identify Object 2 as a neighbor.
  auto diff3 = gr.MoveWithDiff(0xA003, 0, 0);
  REQUIRE(diff3.added == std::set<formid>({ 0xA002, 0xA003 }));
}
