#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class DOOR final : public RecordHeader
{
public:
  static constexpr auto kType = "DOOR";
};

static_assert(sizeof(DOOR) == sizeof(RecordHeader));

}

#pragma pack(pop)
