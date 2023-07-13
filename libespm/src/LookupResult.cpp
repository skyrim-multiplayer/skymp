#include "libespm/LookupResult.h"

namespace espm {

LookupResult::LookupResult(const CombineBrowser* parent_,
                           const RecordHeader* rec_, uint8_t fileIdx_)
  : BrowserInfo(parent_, fileIdx_)
  , rec(rec_)
{
}

}
