#include "libespm/RACE.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

RACE::Data RACE::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t size, const char* data) {
      if (!std::memcmp(type, "DATA", 4)) {
        result.startingHealth = *reinterpret_cast<const float*>(data + 36);
        result.startingMagicka = *reinterpret_cast<const float*>(data + 40);
        result.startingStamina = *reinterpret_cast<const float*>(data + 44);
        result.healRegen = *reinterpret_cast<const float*>(data + 84);
        result.magickaRegen = *reinterpret_cast<const float*>(data + 88);
        result.staminaRegen = *reinterpret_cast<const float*>(data + 92);
        result.unarmedDamage = *reinterpret_cast<const float*>(data + 96);
        result.unarmedReach = *reinterpret_cast<const float*>(data + 100);
      } else if (!std::memcmp(type, "SPLO", 4)) {
        result.spells.emplace(*reinterpret_cast<const uint32_t*>(data));
      }
    },
    compressedFieldsCache);
  return result;
}

}
