#include "libespm/NPC_.h"
#include "libespm/RecordHeaderAccess.h"
#include "libespm/Utils.h"
#include <cstring>

namespace espm {

NPC_::Data NPC_::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "DOFT", 4)) {
        result.defaultOutfitId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "SOFT", 4)) {
        result.sleepOutfitId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "SNAM", 4)) {
        uint32_t formId = *reinterpret_cast<const uint32_t*>(data);
        int8_t rank = *reinterpret_cast<const int8_t*>(data);

        result.factions.push_back({ formId, rank });
      } else if (!std::memcmp(type, "ACBS", 4)) {
        const uint32_t flags = *reinterpret_cast<const uint32_t*>(data);

        result.isEssential = !!(flags & 0x2);
        result.isUnique = !!(flags & 0x20);
        result.isProtected = !!(flags & 0x800);
        result.magickaOffset = *reinterpret_cast<const int16_t*>(data + 4);
        result.staminaOffset = *reinterpret_cast<const int16_t*>(data + 6);
        result.healthOffset = *reinterpret_cast<const int16_t*>(data + 20);
        result.templateDataFlags =
          *reinterpret_cast<const uint16_t*>(data + 18);

      } else if (!std::memcmp(type, "RNAM", 4)) {
        result.race = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "OBND", 4)) {
        if (const auto objectBounds =
              reinterpret_cast<const ObjectBounds*>(data)) {
          result.objectBounds = *objectBounds;
        }
      } else if (!std::memcmp(type, "SPLO", 4)) {
        result.spells.emplace(*reinterpret_cast<const uint32_t*>(data));
      } else if (!std::memcmp(type, "TPLT", 4)) {
        result.baseTemplate = (*reinterpret_cast<const uint32_t*>(data));
      }
    },
    compressedFieldsCache);

  result.objects = utils::GetContainerObjects(this, compressedFieldsCache);
  return result;
}

}
