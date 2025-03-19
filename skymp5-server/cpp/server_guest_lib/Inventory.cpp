#include "Inventory.h"
#include "archives/JsonInputArchive.h"
#include "archives/JsonOutputArchive.h"
#include "archives/SimdJsonInputArchive.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <tuple>

Inventory::Entry Inventory::Entry::FromJson(const simdjson::dom::element& e)
{
  std::string minifiedDump = simdjson::minify(e);
  nlohmann::json j = nlohmann::json::parse(minifiedDump);

  Entry res;
  JsonInputArchive ar(j);
  res.Serialize(ar);
  return res;
}

Inventory::Worn Inventory::Entry::GetWorn() const
{
  bool wornValue = worn_.value_or(false);
  bool wornLeftValue = wornLeft.value_or(false);

  if (wornLeftValue) {
    return Worn::Left;
  }
  if (wornValue) {
    return Worn::Right;
  }
  return Worn::None;
}

void Inventory::Entry::SetWorn(Inventory::Worn worn)
{
  if (worn == GetWorn()) {
    return;
  }

  switch (worn) {
    case Worn::None:
      worn_ = false;
      wornLeft = false;
      break;
    case Worn::Right:
      worn_ = true;
      wornLeft = false;
      break;
    case Worn::Left:
      worn_ = false;
      wornLeft = true;
      break;
    default:
      spdlog::warn("Inventory::SetWorn: unknown worn value {}",
                   static_cast<int>(worn));
      worn_ = false;
      wornLeft = false;
      break;
  }
}

bool Inventory::Entry::EqualExceptCount(const Inventory::Entry& other) const
{
  // GetWorn() instead of direct comparison because of possible false vs
  // nullopt mismatch. Logically it should be the same
  return std::make_tuple(baseId, health, enchantmentId, maxCharge,
                         removeEnchantmentOnUnequip, chargePercent, name, soul,
                         poisonId, poisonCount, GetWorn()) ==
    std::make_tuple(other.baseId, other.health, other.enchantmentId,
                    other.maxCharge, other.removeEnchantmentOnUnequip,
                    other.chargePercent, other.name, other.soul,
                    other.poisonId, other.poisonCount, other.GetWorn());
}

Inventory& Inventory::AddItem(uint32_t baseId, uint32_t count)
{
  return AddItems({ { baseId, count } });
}

Inventory& Inventory::AddItems(const std::vector<Entry>& toAdd)
{
  for (auto& entryToAdd : toAdd) {
    for (auto& entry : entries) {
      if (entry.EqualExceptCount(entryToAdd)) {
        entry.count += entryToAdd.count;
        return *this; // TODO: It seems there is a bug
      }
    }
    entries.push_back(entryToAdd);
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
      if (entry.EqualExceptCount(e)) {
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
  for (auto& entry : entries) {
    if (entry.baseId == baseId) {
      return true;
    }
  }
  return false;
}

uint32_t Inventory::GetItemCount(uint32_t baseId) const
{
  uint32_t sum = 0;
  for (auto& entry : entries) {
    if (entry.baseId == baseId) {
      sum += entry.count;
    }
  }
  return sum;
}

uint32_t Inventory::GetTotalItemCount() const
{
  uint32_t sum = 0;
  for (auto& entry : entries) {
    sum += entry.count;
  }
  return sum;
}

bool Inventory::IsEmpty() const
{
  return entries.empty();
}

nlohmann::json Inventory::ToJson() const
{
  JsonOutputArchive ar;
  const_cast<Inventory*>(this)->Serialize(ar);
  return std::move(ar.j);
}

Inventory Inventory::FromJson(const simdjson::dom::element& element)
{
  SimdJsonInputArchive ar(element);
  Inventory res;
  res.Serialize(ar);
  return res;
}

Inventory Inventory::FromJson(const nlohmann::json& j)
{
  JsonInputArchive ar(j);
  Inventory res;
  res.Serialize(ar);
  return res;
}
