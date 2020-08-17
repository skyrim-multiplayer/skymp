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
  static const EntryExtras emptyExtra;

  for (auto& entry : entries) {
    if (entry.baseId == baseId && entry.extra == emptyExtra) {
      entry.count += count;
      return;
    }
  }

  entries.push_back(Entry{ baseId, count, emptyExtra });
}

bool Inventory::HasItem(uint32_t baseId) const
{
  for (auto& entry : entries)
    if (entry.baseId == baseId)
      return true;
  return false;
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
    ReadEx(jEntry, "baseId", &e.baseId);
    ReadEx(jEntry, "count", &e.count);
  }
  return res;
}