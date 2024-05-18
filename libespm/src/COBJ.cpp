#include "libespm/COBJ.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

COBJ::Data COBJ::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "CNTO", 4)) {
        result.inputObjects.push_back(
          *reinterpret_cast<const InputObject*>(data));
      } else if (!std::memcmp(type, "CNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.outputObjectFormId = formId;
      } else if (!std::memcmp(type, "BNAM", 4)) {
        const auto formId = *reinterpret_cast<const uint32_t*>(data);
        result.benchKeywordId = formId;
      } else if (!std::memcmp(type, "CTDA", 4)) {
        result.conditions.push_back(*reinterpret_cast<const CTDA*>(data));
      }
    },
    compressedFieldsCache);
  return result;
}

}
