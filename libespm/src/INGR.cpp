#include "libespm/INGR.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

INGR::Data INGR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

}
