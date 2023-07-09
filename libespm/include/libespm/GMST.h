#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

// game settings
class GMST : public RecordHeader
{
public:
  static constexpr auto kType = "GMST";

  static constexpr uint32_t kFCombatDistance = 0x00055640;
  static constexpr uint32_t kFMaxArmorRating = 0x00037DEB;
  static constexpr uint32_t kFArmorScalingFactor = 0x00021A72;

  struct Data
  {
    float value = 0.f;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(GMST) == sizeof(RecordHeader));

}

#pragma pack(pop)
