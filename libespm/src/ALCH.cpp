#include "libespm/ALCH.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

ALCH::Data ALCH::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  result.effects = Effects(this).GetData(compressedFieldsCache).effects;
  return result;
}

}
