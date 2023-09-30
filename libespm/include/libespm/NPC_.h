#pragma once
#include "CONT.h"
#include "ObjectBounds.h"
#include "RecordHeader.h"
#include <set>

#pragma pack(push, 1)

namespace espm {

class NPC_ final : public RecordHeader
{
public:
  static constexpr auto kType = "NPC_";

  struct Faction
  {
    uint32_t formId = 0;
    int8_t rank = 0;
  };

  struct Data
  {
    uint32_t defaultOutfitId = 0;
    uint32_t sleepOutfitId = 0;

    std::set<uint32_t> spells = {};

    std::vector<CONT::ContainerObject> objects;
    std::vector<Faction> factions;

    bool isEssential = false;
    bool isProtected = false;

    uint32_t race = 0;
    int16_t healthOffset = 0;
    int16_t magickaOffset = 0;
    int16_t staminaOffset = 0;
    ObjectBounds objectBounds = {};
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(NPC_) == sizeof(RecordHeader));

}

#pragma pack(pop)
