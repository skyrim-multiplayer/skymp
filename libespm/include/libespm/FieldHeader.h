#pragma once
#include <cstdint>

#pragma pack(push, 1)

namespace espm {

struct FieldHeader
{
  char type[4];
  uint16_t dataSize;
};

static_assert(sizeof(FieldHeader) == 6);

}

#pragma pack(pop)
