#include "Inventory.h"
#include "JsonUtils.h"
#include <fmt/format.h>
#include <tuple>

bool operator==(const Inventory::EntryExtras& r,
                const Inventory::EntryExtras& l)
{
  return std::make_tuple(r.health, r.ench.id, r.ench.maxCharge,
                         r.ench.removeOnUnequip, r.poison.count, r.poison.id,
                         r.chargePercent, r.name, r.soul, r.worn) ==
    std::make_tuple(l.health, l.ench.id, l.ench.maxCharge,
                    l.ench.removeOnUnequip, l.poison.count, l.poison.id,
                    l.chargePercent, l.name, l.soul, l.worn);
}

Inventory& Inventory::AddItem(uint32_t baseId, uint32_t count)
{
  return AddItems({ { baseId, count } });
}

Inventory& Inventory::AddItems(const std::vector<Entry>& toAdd)
{
  for (auto& entryToAdd : toAdd) {
    for (auto& entry : entries) {
      if (entry.baseId == entryToAdd.baseId &&
          entry.extra == entryToAdd.extra) {
        entry.count += entryToAdd.count;
        return *this; // TODO: It seems there is a bug
      }
    }
    entries.push_back(
      Entry{ entryToAdd.baseId, entryToAdd.count, entryToAdd.extra });
  }
  return *this;
}

Inventory& Inventory::RemoveItems(const std::vector<Entry>& entries)
{
  auto copy = *this;

  for (auto& e : entries) {
    if (!e.count)
      continue;

    uint32_t remaining = e.count;
    uint32_t totalRemoved = 0;
    for (auto& entry : copy.entries) {
      if (entry.baseId == e.baseId && entry.extra == e.extra) {
        if (entry.count > remaining) {
          entry.count -= remaining;
          totalRemoved += remaining;
          remaining = 0;
          break;
        } else {
          totalRemoved += entry.count;
          remaining -= entry.count;
          entry.count = 0;
        }
      }
    }

    if (totalRemoved != e.count) {
      throw std::runtime_error(
        fmt::format("Source inventory doesn't have enough {:#x} ({} is "
                    "required while {} present)",
                    e.baseId, e.count, totalRemoved));
    }

    // remove empty entries
    copy.entries.erase(
      std::remove_if(copy.entries.begin(), copy.entries.end(),
                     [](const Entry& e) { return e.count == 0; }),
      copy.entries.end());
  }

  *this = copy;
  return *this;
}

bool Inventory::HasItem(uint32_t baseId) const
{
  for (auto& entry : entries)
    if (entry.baseId == baseId)
      return true;
  return false;
}

uint32_t Inventory::GetItemCount(uint32_t baseId) const
{
  uint32_t sum = 0;
  for (auto& entry : entries)
    if (entry.baseId == baseId)
      sum += entry.count;
  return sum;
}

uint32_t Inventory::GetTotalItemCount() const
{
  uint32_t sum = 0;
  for (auto& entry : entries)
    sum += entry.count;
  return sum;
}

bool Inventory::IsEmpty() const
{
  return entries.empty();
}

nlohmann::json Inventory::Entry::ToJson() const
{
  const EntryExtras emptyExtras;

  nlohmann::json obj = { { "baseId", baseId }, { "count", count } };
  if (extra.health != emptyExtras.health)
    obj["health"] = extra.health;
  if (extra.ench.id != emptyExtras.ench.id) {
    obj["enchantmentId"] = extra.ench.id;
    obj["maxCharge"] = extra.ench.maxCharge;
    obj["removeEnchantmentOnUnequip"] = extra.ench.removeOnUnequip;
  }
  if (extra.chargePercent != emptyExtras.chargePercent) {
    obj["chargePercent"] = extra.chargePercent;
  }
  if (extra.name != emptyExtras.name) {
    obj["name"] = extra.name;
  }
  if (extra.soul != emptyExtras.soul) {
    obj["soul"] = extra.soul;
  }
  if (extra.poison.id != emptyExtras.poison.id) {
    obj["poisonId"] = extra.poison.id;
    obj["poisonCount"] = extra.poison.count;
  }
  if (extra.worn == Worn::Left) {
    obj["wornLeft"] = true;
  }
  if (extra.worn == Worn::Right) {
    obj["worn"] = true;
  }
  return obj;
}

Inventory::Entry Inventory::Entry::FromJson(
  const simdjson::dom::element& jEntry)
{
  static JsonPointer baseId("baseId"), count("count"), worn("worn"),
    wornLeft("wornLeft"), health("health"), enchantmentId("enchantmentId"),
    maxCharge("maxCharge"),
    removeEnchantmentOnUnequip("removeEnchantmentOnUnequip"),
    chargePercent("chargePercent"), name("name"), soul("soul"),
    poisonId("poisonId"), poisonCount("poisonCount");

  Entry e;

  ReadEx(jEntry, baseId, &e.baseId);
  ReadEx(jEntry, count, &e.count);

  if (jEntry.at_pointer(health.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, health, &e.extra.health);
  }
  if (jEntry.at_pointer(enchantmentId.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, enchantmentId, &e.extra.ench.id);
  }
  if (jEntry.at_pointer(maxCharge.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, maxCharge, &e.extra.ench.maxCharge);
  }
  if (jEntry.at_pointer(removeEnchantmentOnUnequip.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, removeEnchantmentOnUnequip, &e.extra.ench.removeOnUnequip);
  }
  if (jEntry.at_pointer(chargePercent.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, chargePercent, &e.extra.chargePercent);
  }
  if (jEntry.at_pointer(name.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, name, &e.extra.name);
  }
  if (jEntry.at_pointer(soul.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, soul, &e.extra.soul);
  }
  if (jEntry.at_pointer(poisonId.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, poisonId, &e.extra.poison.id);
  }
  if (jEntry.at_pointer(poisonCount.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, poisonCount, &e.extra.poison.count);
  }

  bool wornValue;
  if (jEntry.at_pointer(worn.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, worn, &wornValue);
  } else {
    wornValue = false;
  }

  bool wornLeftValue;
  if (jEntry.at_pointer(wornLeft.GetData()).error() ==
      simdjson::error_code::SUCCESS) {
    ReadEx(jEntry, wornLeft, &wornLeftValue);
  } else {
    wornLeftValue = false;
  }

  if (wornLeftValue)
    e.extra.worn = Inventory::Worn::Left;
  else if (wornValue)
    e.extra.worn = Inventory::Worn::Right;

  return e;
}

nlohmann::json Inventory::ToJson() const
{
  auto r = nlohmann::json::array();
  for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    r.push_back(entries[i].ToJson());
  return { { "entries", r } };
}

Inventory Inventory::FromJson(simdjson::dom::element& j)
{
  static const JsonPointer entries("entries");

  std::vector<simdjson::dom::element> parsedEntries;
  ReadVector(j, entries, &parsedEntries);

  Inventory res;
  res.entries.resize(parsedEntries.size());
  for (size_t i = 0; i != res.entries.size(); ++i) {
    auto& jEntry = parsedEntries[i];
    auto& e = res.entries[i];
    e = Entry::FromJson(jEntry);
  }

  return res;
}

Inventory Inventory::FromJson(const nlohmann::json& j)
{
  simdjson::dom::parser p;
  simdjson::dom::element parsed = p.parse(j.dump());
  return FromJson(parsed);
}
