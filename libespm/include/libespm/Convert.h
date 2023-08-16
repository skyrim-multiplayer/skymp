#pragma once
#include "RecordHeader.h"

namespace espm {

template <class RecordT>
const RecordT* Convert(const RecordHeader* source)
{
  if (source && source->GetType() == RecordT::kType) {
    return static_cast<const RecordT*>(source);
  }
  return nullptr;
}

}
