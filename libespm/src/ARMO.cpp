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
      }
    },
    compressedFieldsCache);
  if (!hasDNAM) {
    throw std::runtime_error("bad record ARMO? DNAM was not found");
  }
  return result;
}

}
