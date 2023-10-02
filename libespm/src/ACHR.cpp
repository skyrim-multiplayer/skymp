#include "libespm/ACHR.h"
#include "libespm/RecordHeaderAccess.h"
#include <cstring>

namespace espm {

bool ACHR::StartsDead() const noexcept
{
  return this->flags & 0x200;
}

}
