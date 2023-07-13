#pragma once
#include <cstdint>
#include <type_traits>

namespace espm {

enum class GroupType : uint32_t
{
  TOP,
  WORLD_CHILDREN,
  INTERIOR_CELL_BLOCK,
  INTERIOR_CELL_SUBBLOCK,
  EXTERIOR_CELL_BLOCK,
  EXTERIOR_CELL_SUBBLOCK,
  CELL_CHILDREN,
  TOPIC_CHILDREN,
  CELL_PERSISTENT_CHILDREN,
  CELL_TEMPORARY_CHILDREN,
  CELL_VISIBLE_DISTANT_CHILDREN
};

static_assert(static_cast<std::underlying_type_t<GroupType>>(
                GroupType::CELL_VISIBLE_DISTANT_CHILDREN) == 10);

}
