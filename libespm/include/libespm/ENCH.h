#pragma once
#include "Effects.h"
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class ENCH : public RecordHeader
{
public:
  static constexpr auto kType = "ENCH";

  struct Data
  {
    std::vector<Effects::Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(ENCH) == sizeof(RecordHeader));

}

#pragma pack(pop)
