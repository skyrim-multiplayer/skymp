#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class LVLI final : public RecordHeader
{
public:
  static constexpr auto kType = "LVLI";

  enum LeveledItemFlags
  {
    AllLevels = 0x01, //(sets it to calculate for all entries < player level,
                      // choosing randomly from all the entries under)
    Each = 0x02, // (sets it to repeat a check every time the list is called
                 // (if it's called multiple times), otherwise it will use the
                 // same result for all counts.)
    UseAll = 0x04, // (use all entries when the list is called)
    SpecialLoot = 0x08,
  };

  struct Entry
  {
    char type[4] = { 'L', 'V', 'L', 'O' };
    uint16_t dataSize = 0;
    uint32_t level = 0;
    uint32_t formId = 0;
    uint32_t count = 0;
  };

  struct Data
  {
    const char* editorId = "";
    uint8_t chanceNone = 0;
    uint8_t leveledItemFlags = 0;
    uint32_t chanceNoneGlobalId = 0;
    uint8_t numEntries = 0;
    const Entry* entries = nullptr;
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(LVLI) == sizeof(RecordHeader));
static_assert(sizeof(LVLI::Entry) == 18);

}

#pragma pack(pop)
