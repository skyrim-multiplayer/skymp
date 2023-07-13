#pragma once
#include "ActorValue.h"
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class MGEF : public RecordHeader
{
public:
  static constexpr auto kType = "MGEF";

  struct DATA
  {
    // primary actor value
    ActorValue primaryAV = espm::ActorValue::None;
  };

  struct Data
  {
    DATA data;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(MGEF) == sizeof(RecordHeader));

}

#pragma pack(pop)
