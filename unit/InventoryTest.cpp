#include "Inventory.h"
#include <catch2/catch_all.hpp>

TEST_CASE(
  "RemoveItems in case of multiple entries with the same baseId/extras",
  "[Inventory]")
{
  Inventory inv;
  inv.entries.push_back({ 0xf, 10, {} });
  inv.entries.push_back({ 0xf, 10, {} });

  inv.RemoveItems({ { 0xf, 5, {} } });

  REQUIRE(inv.entries.size() == 2);
  REQUIRE(inv.entries[0].count == 5);
  REQUIRE(inv.entries[1].count == 10);
}

TEST_CASE("Similar entries should be merged if needed to remove items",
          "[Inventory]")
{
  Inventory inv;
  inv.entries.push_back({ 0xf, 10, {} });
  inv.entries.push_back({ 0xf, 10, {} });

  inv.RemoveItems({ { 0xf, 15, {} } });

  REQUIRE(inv.entries.size() == 1);
  REQUIRE(inv.entries[0].count == 5);
}

TEST_CASE("Remove all items from similar entries", "[Inventory]")
{
  Inventory inv;
  inv.entries.push_back({ 0xf, 10, {} });
  inv.entries.push_back({ 0xf, 10, {} });

  inv.RemoveItems({ { 0xf, 20, {} } });

  REQUIRE(inv.entries.size() == 0);
}

TEST_CASE("Remove all items from a single entry", "[Inventory]")
{
  Inventory inv;
  inv.entries.push_back({ 0xf, 10, {} });

  inv.RemoveItems({ { 0xf, 10, {} } });

  REQUIRE(inv.entries.size() == 0);
}

TEST_CASE("Not enough items to remove", "[Inventory]")
{
  Inventory inv;
  inv.entries.push_back({ 0xf, 10, {} });
  inv.entries.push_back({ 0xf, 9, {} });

  REQUIRE_THROWS_AS(inv.RemoveItems({ { 0xf, 20, {} } }), std::runtime_error);
}
