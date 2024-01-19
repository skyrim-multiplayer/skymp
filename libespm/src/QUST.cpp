#include "libespm/QUST.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

QUST::Data QUST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4)) {
        result.editorId = data;
      } else if (!std::memcmp(type, "VMAD", 4)) {
        GetScriptData(&result.scriptData, compressedFieldsCache);
      } else if (!std::memcmp(type, "FULL", 4)) {
        result.questName = data;
      } else if (!std::memcmp(type, "DNAM", 4)) {
        result.questData = reinterpret_cast<const QuestData*>(data);
      } else if (!std::memcmp(type, "ENAM", 4)) {
        result.eventName = data;
      } else if (!std::memcmp(type, "QTGL", 4)) {
        result.displayGlobals = *reinterpret_cast<const uint32_t*>(data);
      } else if (!std::memcmp(type, "FLTR", 4)) {
        result.objectWindowFilter = data;
      } else if (!std::memcmp(type, "ANAM", 4)) {
        result.nextAliasId = *reinterpret_cast<const uint32_t*>(data);
      }
    },
    compressedFieldsCache);
  return result;
}

}
