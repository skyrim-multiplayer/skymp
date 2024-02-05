#pragma once
#include "RecordHeader.h"

#pragma pack(push, 1)

namespace espm {

class CTDA final : public RecordHeader
{
  static constexpr auto kType = "CTDA";

  struct Data
  {
    enum OperatorFlags : uint8_t
    {
      EqualTo = 0,
      NotEqualTo = 1,
      GreaterThen = 2,
      GreaterThanOrEqualTo = 3,
      LessThan = 4,
      LessThanOrEqualTo = 5
    };

    enum RunOnTypeFlags : uint32_t
    {
      Subject = 0,
      Target = 1,
      Reference = 2,
      CombatTarget = 3,
      LinkedReference = 4,
      QuestAlias = 5,
      PackageData = 6,
      e_EventData = 7
    };

    struct DefaultData
    {
      uint32_t firstParameter;
      uint32_t secondParameter;
    };
    static_assert(sizeof(DefaultData) == 8);

    struct EventData
    {
      uint16_t eventFunction;
      char eventMember[2];
      uint32_t parameter;
    };
    static_assert(sizeof(EventData) == 8);

    OperatorFlags operatorFlags;
    uint8_t unknown[3];
    float comparisonValue;
    uint16_t functionIndex;
    const char* additionalData;
    RunOnTypeFlags runOnType;
    uint32_t referenceID;
    uint32_t unknown2;

    bool IsGetEventData()
    {
      return functionIndex == 4672;
    }

    DefaultData GetDefaultData()
    {
      return *reinterpret_cast<const DefaultData*>(additionalData);
    }

    EventData GetEventData()
    {
      return *reinterpret_cast<const EventData*>(additionalData);
    }
  };

  Data GetData(CompressedFieldsCache& compressedFieldsCache) const noexcept;
};

static_assert(sizeof(CTDA) == sizeof(RecordHeader));

}
