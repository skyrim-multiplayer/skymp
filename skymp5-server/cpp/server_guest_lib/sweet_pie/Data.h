#pragma once
#include "NiPoint3.h"
#include "Space.h"
#include <set>

namespace sweetpie {
using ID = uint16_t;
using Score = float;

enum InvalidId : ID
{
  InvalidID = (ID)~0
};

struct Data
{
  ID id = InvalidID;
  Score score = 0;
  Space area;

  _NODISCARD constexpr bool operator==(const Data& right);
  _NODISCARD constexpr bool operator!=(const Data& right);
  _NODISCARD constexpr bool operator<(const Data& right);
  _NODISCARD constexpr bool operator>(const Data& right);
  _NODISCARD constexpr bool operator<=(const Data& right);
  _NODISCARD constexpr bool operator>=(const Data& right);

  constexpr operator bool();
  operator int() = delete;
};
}
