#include "libespm/ENCH.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

ENCH::Data ENCH::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

}
