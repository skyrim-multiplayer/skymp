#include "libespm/LVLI.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

LVLI::Data LVLI::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        result.editorId = data;
      } else if (!std::memcmp(type, "LVLF", 4)) {
        result.leveledItemFlags = *reinterpret_cast<const uint8_t*>(data);
      } else if (!std::memcmp(type, "LVLG", 4)) {
        result.chanceNoneGlobalId = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "LVLD", 4)) {
        result.chanceNone = *reinterpret_cast<const uint8_t*>(data);
      } else if (!std::memcmp(type, "LLCT", 4)) {
        result.numEntries = *reinterpret_cast<const uint8_t*>(data);
        result.entries = reinterpret_cast<const Entry*>(data + 1);
      }
    },
    compressedFieldsCache);
  return result;
}

}
