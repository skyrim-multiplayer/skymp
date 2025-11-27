#include "libespm/ARMO.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

ARMO::Data ARMO::GetData(CompressedFieldsCache& compressedFieldsCache) const
{
  Data result;
  bool hasDNAM = false;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EITM", 4)) {
        result.enchantmentFormId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "DATA", 4)) {
        result.baseValue = *reinterpret_cast<const uint32_t*>(data);
        result.weight = *reinterpret_cast<const float*>(data + 4);
      } else if (!std::memcmp(type, "DNAM", 4)) {
        hasDNAM = true;
        result.baseRatingX100 = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "ETYP", 4)) {
        result.equipSlotId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "BODT", 4) && dataSize >= 8) {
        result.bodt.present = true;
        result.bodt.bodyPartFlags = *reinterpret_cast<const uint32_t*>(data);
        result.bodt.skill = *reinterpret_cast<const uint32_t*>(data + 4);
      } else if (!std::memcmp(type, "BOD2", 4) && dataSize >= 8) {
        result.bod2.present = true;
        result.bod2.bodyPartFlags = *reinterpret_cast<const uint32_t*>(data);
        result.bod2.skill = *reinterpret_cast<const uint32_t*>(data + 4);
      }
    },
    compressedFieldsCache);
  if (!hasDNAM) {
    throw std::runtime_error("bad record ARMO? DNAM was not found");
  }
  return result;
}

}
