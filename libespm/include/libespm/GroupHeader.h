#pragma once
#include "GroupType.h"

#pragma pack(push, 1)

namespace espm {

class GroupHeader
{
  friend class Browser;

public:
  bool GetXY(int16_t& outX, int16_t& outY) const noexcept;
  const char* GetRecordsType() const noexcept;
  bool GetBlockNumber(int32_t& outBlockNum) const noexcept;
  bool GetSubBlockNumber(int32_t& outSubBlockNum) const noexcept;
  bool GetParentWRLD(uint32_t& outId) const noexcept;
  bool GetParentCELL(uint32_t& outId) const noexcept;
  bool GetParentDIAL(uint32_t& outId) const noexcept;
  uint32_t GetGroupLabelAsUint() const noexcept;
  GroupType GetGroupType() const noexcept;

private:
  GroupHeader() = delete;
  GroupHeader(const GroupHeader&) = delete;
  void operator=(const GroupHeader&) = delete;

private:
  char label[4];
  GroupType grType;

  uint8_t day;
  uint8_t months;
  uint16_t unknown;
  uint16_t version;
  uint16_t unknown2;
};

static_assert(sizeof(GroupType) == 4);
static_assert(sizeof(GroupHeader) == 16);

}

#pragma pack(pop)
