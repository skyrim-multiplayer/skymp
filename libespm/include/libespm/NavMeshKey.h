#pragma once
#include "CellOrGridPos.h"
#include "Utils.h"

namespace espm {
class NavMeshKey
{
public:
  NavMeshKey(uint32_t worldSpaceId, CellOrGridPos cellOrGridPos);

  operator uint64_t() const noexcept;

private:
  const uint64_t value;
};

}
