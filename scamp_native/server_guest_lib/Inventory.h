#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>
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

  class EntryExtras
  {
  public:
    float health = 1.f;

    struct
    {
      uint32_t id = 0;
      float maxCharge = 0.f;
      bool removeOnUnequip = false;
    } ench;

    struct
    {
      uint32_t id = 0;
      uint32_t count = 0;
    } poison;

    float chargePercent = 0.f;

    std::string name;

    uint8_t soul = 0;

    Worn worn = Worn::None;

    friend bool operator==(const EntryExtras& r, const EntryExtras& l);
  };

  void AddItem(uint32_t baseId, uint32_t count);
  bool HasItem(uint32_t baseId) const;

  nlohmann::json ToJson() const;

  // Doesn't parse extra data currently
  static Inventory FromJson(simdjson::dom::element& element);

  class Entry
  {
  public:
    nlohmann::json ToJson() const;

    uint32_t baseId = 0;
    uint32_t count = 0;
    EntryExtras extra;
  };

  std::vector<Entry> entries;
};