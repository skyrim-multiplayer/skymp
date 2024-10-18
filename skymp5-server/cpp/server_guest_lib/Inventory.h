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

  // Doesn't parse extra data currently
  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("entries", entries);
  }

  // TODO: get rid of this in favor of Serialize
  nlohmann::json ToJson() const;
  static Inventory FromJson(simdjson::dom::element& element);
  static Inventory FromJson(const nlohmann::json& j);

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
        .Serialize("removeEnchantmentOnUnequip", removeEnchantmentOnUnequip)
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
    static Entry FromJson(const simdjson::dom::element& e);

    Worn GetWorn() const;
    void SetWorn(Worn worn);
    bool EqualExceptCount(const Entry& other) const;

    friend bool operator==(const Entry& lhs, const Entry& rhs)
    {
      return lhs.EqualExceptCount(rhs) && lhs.count == rhs.count;
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
