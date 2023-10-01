#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class ACHR final : public RecordHeader
{
public:
  static constexpr auto kType = "ACHR";

  bool StartsDead() const noexcept;
};

static_assert(sizeof(ACHR) == sizeof(RecordHeader));

}

#pragma pack(pop)
