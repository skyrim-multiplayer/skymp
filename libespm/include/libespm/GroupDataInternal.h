#pragma once
#include <vector>

namespace espm {

struct GroupDataInternal
{
  // Records and GRUPs
  // Pointing to type (record type or "GRUP"), it's RecordHeader/GroupHeader -
  // 4 bytes
  std::vector<const void*> subs;
};

}
