#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class TES4 final : public RecordHeader
{
public:
  static constexpr auto kType = "TES4";

  // Header
  struct Header
  {
    float version = 0;
    int32_t numRecords = 0;
    uint32_t nextObjectId = 0;
  };

  static_assert(sizeof(Header) == 12);

  struct Data
  {
    const Header* header = nullptr;
    const char* author = "";
    const char* description = "";
    std::vector<const char*> masters;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(TES4) == sizeof(RecordHeader));

}

#pragma pack(pop)
