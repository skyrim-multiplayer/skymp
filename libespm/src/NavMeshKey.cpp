#include "libespm/NavMeshKey.h"
#include "libespm/EspmUtils.h"

namespace espm {
NavMeshKey::NavMeshKey(uint32_t worldSpaceId, CellOrGridPos cellOrWorld)
  : value(utils::MakeUInt64(worldSpaceId, cellOrWorld.cellId))
{
}

NavMeshKey::operator uint64_t() const noexcept
{
  return value;
}

}
