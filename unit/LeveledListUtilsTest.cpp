#include "LeveledListUtils.h"
#include "libespm/Loader.h"
#include "libespm/espm.h"
#include <algorithm>
#include <atomic>
#include <catch2/catch_all.hpp>
#include <numeric>
#include <thread>

extern espm::Loader l;

using namespace LeveledListUtils;
using namespace std::chrono_literals;

TEST_CASE("Bug", "[espm]")
{
  espm::CompressedFieldsCache dummyCache;

  auto chestId = 0x774bf;

  std::atomic<bool> finished = false;

  std::thread([&] {
    auto was = std::chrono::system_clock::now();

    while (finished.load() == false) {
      std::this_thread::sleep_for(1ms);

      auto now = std::chrono::system_clock::now();
      if (now - was > 10s) {
        printf(
          "EvaluateListRecurse didn't finish for a single container after 10s "
          "of waiting. Consider it's an infinite loop\n");
        std::abort();
      }
    }
  }).detach();

  auto chest =
    espm::Convert<espm::CONT>(l.GetBrowser().LookupById(chestId).rec);
  auto objects = chest->GetData(dummyCache).objects;
  std::vector<espm::LookupResult> leveled;
  for (auto obj : objects) {
    leveled.push_back(l.GetBrowser().LookupById(obj.formId));
  }

  for (auto lvli : leveled) {
    for (int i = 0; i < 100; ++i) {
      const int pcLevel = 1, count = 1;
      auto result =
        EvaluateListRecurse(l.GetBrowser(), lvli, count, pcLevel, nullptr);
    }
  }
  finished = true;
}

TEST_CASE("Evaluate LItemFoodCabbage75", "[espm]")
{
  auto LItemFoodCabbage75 = 0x10bf6a;
  auto FoodCabbage = 0x64b3f;
  auto& br = l.GetBrowser();
  auto leveledList = br.LookupById(LItemFoodCabbage75);

  int totalItems = 0;

  for (int i = 0; i < 100000; ++i) {
    auto res = EvaluateList(br, leveledList);
    if (res.size() > 0) {
      REQUIRE(res.size() == 1);
      REQUIRE(res[0].formId == FoodCabbage);
      REQUIRE(res[0].count == 1);
      totalItems += res[0].count;
    }
  }

  REQUIRE(totalItems > 74000);
  REQUIRE(totalItems < 76000);
}

TEST_CASE("Evaluate recurse LItemFoodCabbage", "[espm]")
{
  auto LItemFoodCabbage = 0x10bf6e;
  auto FoodCabbage = 0x64b3f;
  auto& br = l.GetBrowser();
  auto leveledList = br.LookupById(LItemFoodCabbage);

  for (int i = 0; i < 100; i++) {
    auto res = EvaluateListRecurse(br, leveledList);
    REQUIRE(res.size() == 1);
    REQUIRE(res[FoodCabbage] >= 1);
    REQUIRE(res[FoodCabbage] <= 5);
  }

  // "Calculate for each" flag is not set
  auto r = EvaluateListRecurse(br, leveledList, 10);
  REQUIRE(r.size() == 1);
  REQUIRE(r[FoodCabbage] % 10 == 0);
}

TEST_CASE("Evaluate LootGoldChange25", "[espm]")
{
  auto LootGoldChange25 = 0x4f78d;
  auto Gold001 = 0xf;
  auto& br = l.GetBrowser();
  auto leveledList = br.LookupById(LootGoldChange25);

  int n[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  for (int i = 0; i < 100000; ++i) {
    auto res = EvaluateList(br, leveledList);
    if (res.size() == 0) {
      n[0]++;
    }
    if (res.size() > 0) {
      REQUIRE(res.size() == 1);
      REQUIRE(res[0].formId == Gold001);
      n[res[0].count]++;
    }
  }

  int initial_sum = 0;

  REQUIRE(std::accumulate(n, n + 10, initial_sum) == 100000);
  REQUIRE(abs(std::accumulate(n + 1, n + 10, initial_sum) / 9 - n[1]) <
          300); // difference between n[1] and avg(1..9) should be < 300
}

TEST_CASE("Evaluate DeathItemDraugr", "[espm]")
{
  auto DeathItemDraugr = 0x3ad7f;
  auto& br = l.GetBrowser();
  auto leveledList = br.LookupById(DeathItemDraugr);

  auto res = EvaluateList(br, leveledList);
  REQUIRE(res.size() == 5);
}

TEST_CASE("Evaluate LItemWeaponDaggerTown", "[espm]")
{
  auto LItemWeaponDaggerTown = 0x17177;
  auto& br = l.GetBrowser();
  auto leveledList = br.LookupById(LItemWeaponDaggerTown);

  // Normally this leveled list returns one of daggers (iron, steel, etc)
  auto res = EvaluateListRecurse(br, leveledList, 1);
  REQUIRE(res.size() == 1);

  // But if we increase count, we will get a lot of different daggers
  // The reason is "Each" flag is set
  res = EvaluateListRecurse(br, leveledList, 1000);

  REQUIRE(res.size() == 5);
  REQUIRE(res[0x1397e] > 100);
  REQUIRE(res[0x13986] > 100);
  REQUIRE(res[0x1398e] > 30);
  REQUIRE(res[0x13996] > 30);
  REQUIRE(res[0x1399e] > 30);

  // PlayerCharacter's level is 1. Only IronDagger should be generated
  res = EvaluateListRecurse(br, leveledList, 1000, 1);
  REQUIRE(res.size() == 1);
  REQUIRE(res[0x1397e] == 1000);
}
