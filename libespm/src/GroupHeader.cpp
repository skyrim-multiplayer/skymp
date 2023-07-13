#include "libespm/GroupHeader.h"

namespace espm {

bool GroupHeader::GetXY(int16_t& outX, int16_t& outY) const noexcept
{
  if (grType == GroupType::EXTERIOR_CELL_BLOCK ||
      grType == GroupType::EXTERIOR_CELL_SUBBLOCK) {
    outY = *reinterpret_cast<const int16_t*>(label);
    outX = *reinterpret_cast<const int16_t*>(label + 2);
    return true;
  }
  return false;
}

const char* GroupHeader::GetRecordsType() const noexcept
{
  if (grType != GroupType::TOP) {
    return nullptr;
  }
  return label;
}

bool GroupHeader::GetBlockNumber(int32_t& outBlockNum) const noexcept
{
  if (grType != GroupType::INTERIOR_CELL_BLOCK) {
    return false;
  }
  outBlockNum = *reinterpret_cast<const int32_t*>(label);
  return true;
}

bool GroupHeader::GetSubBlockNumber(int32_t& outSubBlockNum) const noexcept
{
  if (grType != GroupType::INTERIOR_CELL_SUBBLOCK) {
    return false;
  }
  outSubBlockNum = *reinterpret_cast<const int32_t*>(label);
  return true;
}

bool GroupHeader::GetParentWRLD(uint32_t& outId) const noexcept
{
  if (grType != GroupType::WORLD_CHILDREN) {
    return false;
  }
  outId = *reinterpret_cast<const uint32_t*>(label);
  return true;
}

bool GroupHeader::GetParentCELL(uint32_t& outId) const noexcept
{
  if (grType != GroupType::CELL_CHILDREN &&
      grType != GroupType::CELL_PERSISTENT_CHILDREN &&
      grType != GroupType::CELL_TEMPORARY_CHILDREN &&
      grType != GroupType::CELL_VISIBLE_DISTANT_CHILDREN) {
    return false;
  }
  outId = *reinterpret_cast<const uint32_t*>(label);
  return true;
}

bool GroupHeader::GetParentDIAL(uint32_t& outId) const noexcept
{
  if (grType != GroupType::TOPIC_CHILDREN)
    return false;
  outId = *reinterpret_cast<const uint32_t*>(label);
  return true;
}

uint32_t espm::GroupHeader::GetGroupLabelAsUint() const noexcept
{
  return *reinterpret_cast<const uint32_t*>(label);
}

GroupType GroupHeader::GetGroupType() const noexcept
{
  return grType;
}

}
