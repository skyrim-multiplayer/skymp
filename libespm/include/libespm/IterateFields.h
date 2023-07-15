#pragma once
#include "RecordHeader.h"
#include <functional>

namespace espm {

class RecordHeader;
class CompressedFieldsCache;
using IterateFieldsCallback =
  std::function<void(const char* type, uint32_t size, const char* data)>;

void IterateFields_(const RecordHeader* rec, const IterateFieldsCallback& f,
                    CompressedFieldsCache& compressedFieldsCache);

}
