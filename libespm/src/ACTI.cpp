#include "libespm/ACTI.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

ACTI::Data ACTI::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  Data result;
  GetScriptData(&result.scriptData, compressedFieldsCache);
  return result;
}

}
