#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class CONT : public RecordHeader
{
public:
  static constexpr auto kType = "CONT";

  struct ContainerObject
  {
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    const char* fullName = "";
    std::vector<ContainerObject> objects;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};
static_assert(sizeof(CONT) == sizeof(RecordHeader));

}

#pragma pack(pop)
