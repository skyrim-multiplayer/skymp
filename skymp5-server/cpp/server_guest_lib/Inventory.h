#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>
#include <tuple>
#include <vector>

class Inventory
{
public:
  enum class Worn
  {
    None = 0,
    Right,
    Left
  };

  // class EntryExtras
  // {
  // public:
  //   float health = 1.f;

  //   struct
  //   {
  //     uint32_t id = 0;
  //     float maxCharge = 0.f;
  //     bool removeOnUnequip = false;
  //   } ench;

  //   struct
  //   {
  //     uint32_t id = 0;
  //     uint32_t count = 0;
  //   } poison;

  //   float chargePercent = 0.f;

  //   std::string name;

  //   uint8_t soul = 0;

  //   bool worn_ = false;
  //   bool wornLeft = false;

  //   friend bool operator==(const EntryExtras& r, const EntryExtras& l);

  //   friend bool operator!=(const EntryExtras& r, const EntryExtras& l)
  //   {
  //     return !(r == l);
  //   }
  // };

  // Doesn't parse extra data currently
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("entries", entries);
  }

  // // TODO: get rid of this in favor of Serialize
  // nlohmann::json ToJson() const;

  // // TODO: get rid of this in favor of Serialize
  // static Inventory FromJson(simdjson::dom::element& element);
  // static Inventory FromJson(const nlohmann::json& j);

  class Entry
  {
  public:
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("baseId", baseId)
        .Serialize("count", count)
        .Serialize("health", health)
        .Serialize("enchantmentId", enchantmentId)
        .Serialize("maxCharge", maxCharge)
        .Serialize("removeEnchantmentOnUnequip", removeOnUnequip)
        .Serialize("chargePercent", chargePercent)
        .Serialize("name", name)
        .Serialize("soul", soul)
        .Serialize("poisonId", poisonId)
        .Serialize("poisonCount", poisonCount)
        .Serialize("worn", worn_)
        .Serialize("wornLeft", wornLeft);
    }

    // TODO: get rid of this in favor of Serialize
    // nlohmann::json ToJson() const;

    uint32_t baseId = 0;
    uint32_t count = 0;

    // extras
    std::optional<float> health;
    std::optional<uint32_t> enchantmentId;
    std::optional<float> maxCharge;
    std::optional<bool> removeEnchantmentOnUnequip;
    std::optional<float> chargePercent;
    std::optional<std::string> name;
    std::optional<uint8_t> soul;
    std::optional<uint32_t> poisonId;
    std::optional<uint32_t> poisonCount;
    std::optional<bool> worn_;
    std::optional<bool> wornLeft;

    // TODO: get rid of this in favor of Serialize
    // static Entry FromJson(const simdjson::dom::element& e);

    friend bool operator==(const Entry& lhs, const Entry& rhs)
    {
      return std::make_tuple(lhs.baseId, lhs.count, lhs.health,
                             lfs.enchantmentId, lfs.maxCharge,
                             lfs.removeEnchantmentOnUnequip, lfs.chargePercent,
                             lfs.name, lfs.soul, lfs.poisonId, lfs.poisonCount,
                             lfs.worn_, lfs.wornLeft) ==
        std::make_tuple(rhs.baseId, rhs.count, rhs.extra, rhs.health,
                        rhs.enchantmentId, rhs.maxCharge,
                        rhs.removeEnchantmentOnUnequip, rhs.chargePercent,
                        rhs.name, rhs.soul, rhs.poisonId, rhs.poisonCount,
                        rhs.worn_, rhs.wornLeft);
    }

    friend bool operator!=(const Entry& lhs, const Entry& rhs)
    {
      return !(lhs == rhs);
    }
  };

  Inventory& AddItem(uint32_t baseId, uint32_t count);
  Inventory& AddItems(const std::vector<Entry>& entries);
  Inventory& RemoveItems(const std::vector<Entry>& entries);
  bool HasItem(uint32_t baseId) const;
  uint32_t GetItemCount(uint32_t baseId) const;
  uint32_t GetTotalItemCount() const;
  bool IsEmpty() const;

  std::vector<Entry> entries;

  friend bool operator==(const Inventory& lhs, const Inventory& rhs)
  {
    return lhs.entries == rhs.entries;
  }

  friend bool operator!=(const Inventory& lhs, const Inventory& rhs)
  {
    return !(lhs == rhs);
  }
};
