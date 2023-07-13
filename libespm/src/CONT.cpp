#include "libespm/CONT.h"
#include "libespm/RecordHeaderAccess.h"
#include "libespm/Utils.h"
#include <cstring>

namespace espm {

CONT::Data CONT::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "EDID", 4))
        result.editorId = data;
      else if (!std::memcmp(type, "FULL", 4))
        result.fullName = data;
    },
    compressedFieldsCache);
  result.objects = utils::GetContainerObjects(this, compressedFieldsCache);
  return result;
}

}
