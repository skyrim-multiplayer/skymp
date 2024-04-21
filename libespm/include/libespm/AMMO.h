#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class AMMO final : public RecordHeader
{
public:
  static constexpr auto kType = "AMMO";

  struct Data
  {
    float weight;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const;
};

static_assert(sizeof(AMMO) == sizeof(RecordHeader));

}

#pragma pack(pop)
