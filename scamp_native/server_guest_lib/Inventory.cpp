#include "Inventory.h"
#include "JsonUtils.h"
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

void Inventory::AddItem(uint32_t baseId, uint32_t count)
{
  return AddItems({ { baseId, count } });
}

void Inventory::AddItems(const std::vector<Entry>& toAdd)
{
  for (auto& entryToAdd : toAdd) {
    for (auto& entry : entries) {
      if (entry.baseId == entryToAdd.baseId &&
          entry.extra == entryToAdd.extra) {
        entry.count += entryToAdd.count;
        return;
      }
    }
    entries.push_back(
      Entry{ entryToAdd.baseId, entryToAdd.count, entryToAdd.extra });
  }
}

void Inventory::RemoveItems(const std::vector<Entry>& entries)
{
  auto copy = *this;

  for (auto& e : entries) {
    if (!e.count)
      continue;

    auto matchingEntry = std::find_if(
      copy.entries.begin(), copy.entries.end(), [&](const Entry& sub) {
        return sub.baseId == e.baseId && sub.extra == e.extra;
      });

    auto count =
      matchingEntry == copy.entries.end() ? 0 : matchingEntry->count;

    if (count < e.count) {
      std::stringstream err;
      err << "Source inventory doesn't have enough 0x" << std::hex << e.baseId
          << " (" << e.count << " is required while " << count << " present)";
      throw std::runtime_error(err.str());
    }

    matchingEntry->count -= e.count;
    if (matchingEntry->count == 0) {
      copy.entries.erase(matchingEntry);
    }
  }

  *this = copy;
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
    obj["chargePercent"] = emptyExtras.chargePercent;
  }
  if (extra.name != emptyExtras.name) {
    obj["name"] = emptyExtras.name;
  }
  if (extra.soul != emptyExtras.soul) {
    obj["soul"] = emptyExtras.soul;
  }
  if (extra.poison.id != emptyExtras.poison.id) {
    obj["poisonId"] = emptyExtras.poison.id;
    obj["poisonCount"] = emptyExtras.poison.count;
  }
  if (extra.worn == Worn::Left) {
    obj["wornLeft"] = true;
  }
  if (extra.worn == Worn::Right) {
    obj["worn"] = true;
  }
  return obj;
}

Inventory::Entry Inventory::Entry::FromJson(simdjson::dom::element& jEntry)
{
  Entry e;
  ReadEx(jEntry, "baseId", &e.baseId);
  ReadEx(jEntry, "count", &e.count);
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
  std::vector<simdjson::dom::element> entries;
  ReadVector(j, "entries", &entries);

  Inventory res;
  res.entries.resize(entries.size());
  for (size_t i = 0; i != res.entries.size(); ++i) {
    auto& jEntry = entries[i];
    auto& e = res.entries[i];
    e = Entry::FromJson(jEntry);
  }
  return res;
}