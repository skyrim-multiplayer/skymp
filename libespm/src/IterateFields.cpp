#include "libespm/IterateFields.h"
#include "libespm/RecordHeaderAccess.h"

namespace espm {

void IterateFields_(const RecordHeader* rec, const IterateFieldsCallback& f,
                    CompressedFieldsCache& compressedFieldsCache)
{
  RecordHeaderAccess::IterateFields(rec, f, compressedFieldsCache);
}

}
