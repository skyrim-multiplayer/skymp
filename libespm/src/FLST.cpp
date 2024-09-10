#include "libespm/FLST.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

FLST::Data FLST::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "LNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.formIds.push_back(formId);
      }
    },
    compressedFieldsCache);
  return result;
}

}
