#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

struct ObjectBounds;

class TREE final : public RecordHeader
{
public:
  static constexpr auto kType = "TREE";

  struct Data
  {
    const char* editorId = "";
    const char* fullName = "";
    const ObjectBounds* bounds = nullptr;
    uint32_t resultItem = 0;
    uint32_t useSound = 0;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(TREE) == sizeof(RecordHeader));

}

#pragma pack(pop)
