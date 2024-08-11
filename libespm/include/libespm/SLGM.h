#pragma once
#include "RecordHeader.h"
#include <string>
#include <vector>
#include <array>

struct SLGM
{
  std::string editorID;
  std ::array<int16_t, 6> objectBounds;
  std::string itemName;
  std::string model;
  std::vector<std::array<float, 3>> modelData;
  uint32_t numKeywords;
  std::vector<uint32_t> keywords;
  uint8_t currentSoul;
  struct Data
  {
    uint32_t baseValue;
    float weight;
  } data;
  uint8_t soulCapacity;
  uint32_t filledGem;
  uint32_t sound;
};

#pragma pack(push, 1)

namespace espm {

class SLGM final : public RecordHeader
{
public:
  static constexpr auto kType = "SLGM";

  struct Data
  {
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(SLGM) == sizeof(RecordHeader));

}

#pragma pack(pop)

