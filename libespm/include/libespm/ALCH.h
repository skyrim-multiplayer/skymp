#pragma once
#include "RecordHeader.h"
#include "Effects.h"

#pragma pack(push, 1)

namespace espm {

class ALCH : public RecordHeader
{
public:
  static constexpr auto kType = "ALCH";

  struct Data
  {
    std::vector<Effects::Effect> effects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(ALCH) == sizeof(RecordHeader));

}

#pragma pack(pop)
