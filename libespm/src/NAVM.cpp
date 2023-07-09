#include "libespm/NAVM.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

NAVM::Vertices::Vertices(const void* nvnmField_)
  : nvnmField(nvnmField_)
{
  numVerticesPtr = reinterpret_cast<const int32_t*>(
    reinterpret_cast<const uint8_t*>(nvnmField) + 16);

  beginPtr = reinterpret_cast<const std::array<float, 3>*>(
    reinterpret_cast<const uint8_t*>(nvnmField) + 20);
}

const std::array<float, 3>* NAVM::Vertices::begin() const noexcept
{
  return beginPtr;
}

const std::array<float, 3>* NAVM::Vertices::end() const noexcept
{
  return beginPtr + (*numVerticesPtr);
}

NAVM::Data NAVM::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  RecordHeaderAccess::IterateFields(
    this,
    [&](const char* type, uint32_t dataSize, const char* data) {
      if (!std::memcmp(type, "NVNM", 4)) {
        result.worldSpaceId = *reinterpret_cast<const uint32_t*>(
          (reinterpret_cast<const uint8_t*>(data) + 8));
        result.cellOrGridPos = *reinterpret_cast<const CellOrGridPos*>(
          (reinterpret_cast<const uint8_t*>(data) + 12));
        result.vertices.reset(new Vertices(data));
      }
    },
    compressedFieldsCache);
  return result;
}

}
