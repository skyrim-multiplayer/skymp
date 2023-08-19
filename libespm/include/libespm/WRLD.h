#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class WRLD final : public RecordHeader
{
public:
  static constexpr auto kType = "WRLD";

  enum Flags
  {
    SmallWorld = 0x01,
    CantFastTravelFromHere = 0x02,
    unknown = 0x04,
    NoLODWater = 0x08,
    NoLandscape = 0x10,
    NoSky = 0x20,
    FixedDimensions = 0x40,
    NoGrass = 0x80
  };

  struct Data
  {
    uint8_t flags = 0;
  };

  Data GetData(CompressedFieldsCache& cache) const noexcept;
};

}

#pragma pack(pop)
