#pragma once
#include <cstdint>

namespace espm {

// Not a record type, but a field type commonly used across different record
// types
struct CTDA
{
  enum class Operator : uint8_t
  {
    EqualTo = 0,
    NotEqualTo = 1,
    GreaterThen = 2,
    GreaterThenOrEqualTo = 3,
    LessThen = 4,
    LessThenOrEqualTo = 5
  };

  enum class Flags : uint8_t
  {
    ANDORDEFAULT = 0x00,
    OR = 0x01,
    Parameters = 0x02,
    UseGlobal = 0x04,
    UsePackData = 0x08,
    SwapSubject = 0x10
  };

  enum class RunOnTypeFlags : uint32_t
  {
    Subject = 0,
    Target = 1,
    Reference = 2,
    CombatTarget = 3,
    LinkedReference = 4,
    QuestAlias = 5,
    PackageData = 6,
    EventData = 7
  };

  struct DefaultData
  {
    uint32_t firstParameter = 0;
    uint32_t secondParameter = 0;
  };
  static_assert(sizeof(DefaultData) == 8);

  struct EventData
  {
    uint16_t eventFunction = 0;
    char eventMember[2];
    uint32_t parameter = 0;
  };
  static_assert(sizeof(EventData) == 8);

  uint8_t operatorFlag = 0;
  uint8_t unknown[3];
  float comparisonValue = 0;
  uint16_t functionIndex = 0;
  uint8_t padding[2];

  // Default/Event data
  uint8_t functionData[8];

  RunOnTypeFlags runOnType;
  int32_t reference = 0;
  int32_t unknown2 = 0;

  Operator GetOperator()
  {
    uint8_t firstBit = static_cast<uint8_t>((operatorFlag & 0x80) ? 4 : 0);
    uint8_t secondBit = static_cast<uint8_t>((operatorFlag & 0x40) ? 2 : 0);
    uint8_t thirdBit = static_cast<uint8_t>((operatorFlag & 0x20) ? 1 : 0);

    return static_cast<Operator>(firstBit + secondBit + thirdBit);
  }

  Flags GetFlags()
  {
    uint8_t firstBit = static_cast<uint8_t>((operatorFlag & 0x10) ? 16 : 0);
    uint8_t secondBit = static_cast<uint8_t>((operatorFlag & 0x08) ? 8 : 0);
    uint8_t thirdBit = static_cast<uint8_t>((operatorFlag & 0x04) ? 4 : 0);
    uint8_t fourthBit = static_cast<uint8_t>((operatorFlag & 0x02) ? 2 : 0);
    uint8_t fifthBit = static_cast<uint8_t>((operatorFlag & 0x01) ? 1 : 0);

    return static_cast<Flags>(firstBit + secondBit + thirdBit + fourthBit +
                              fifthBit);
  }

  bool IsGetIsRace() { return functionIndex == 69; }
  bool IsGetItemCount() { return functionIndex == 47; }
  bool IsGetEventData() { return functionIndex == 576; }
  bool IsHasPerk() { return functionIndex == 448; }

  DefaultData GetDefaultData()
  {
    return *reinterpret_cast<const DefaultData*>(functionData);
  }

  EventData GetEventData()
  {
    return *reinterpret_cast<const EventData*>(functionData);
  }
};
static_assert(sizeof(CTDA) == 32);

}
