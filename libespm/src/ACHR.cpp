#include "libespm/ACHR.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

bool ACHR::StartsDead() const noexcept
{
  return this->flags & 0x200;
}

ACHR::Data ACHR::GetData(
  CompressedFieldsCache& compressedFieldsCache) const noexcept
{
  REFR::Data data =
    reinterpret_cast<const espm::REFR*>(this)->GetData(compressedFieldsCache);

  ACHR::Data res;
  static_cast<REFR::Data&>(res) = data;

  return res;
}

}
