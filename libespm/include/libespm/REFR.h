#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class REFR : public RecordHeader
{
public:
  static constexpr auto kType = "REFR";

  struct LocationalData
  {
    float pos[3];
    float rotRadians[3];
  };

  struct DoorTeleport
  {
    uint32_t destinationDoor = 0;
    float pos[3];
    float rotRadians[3];
  };

  struct ActivationParentInfo
  {
    uint32_t refrId = 0;
    float delay = 0.f;
  };

  struct Data
  {
    uint32_t baseId = 0;
    float scale = 1;
    const LocationalData* loc = nullptr;
    const DoorTeleport* teleport = nullptr;
    const float* boundsDiv2 = nullptr;
    uint32_t count = 0;
    uint8_t isParentActivationOnly = 0;
    std::vector<ActivationParentInfo> activationParents;
    uint32_t linkedRefKeywordId = 0;
    uint32_t linkedRefId = 0;
    uint32_t ownerFaction = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(REFR) == sizeof(RecordHeader));

}

#pragma pack(pop)
