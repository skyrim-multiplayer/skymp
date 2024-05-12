#include "libespm/COBJ.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>
#include <iostream>

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
        std::cout << "------------------------" << std::endl;
        Byte* byteData = ((Byte*)data);
        std::cout << sizeof(byteData) << std::endl;
        std::cout << "------------------------" << std::endl;
        result.conditions.push_back(*reinterpret_cast<const CTDA*>(data));
      } else if (!std::memcmp(type, "NAM1", 4)) {
        const auto count = *reinterpret_cast<const uint16_t*>(data);
        result.outputCount = count;
      }
    },
    compressedFieldsCache);
  return result;
}

}
