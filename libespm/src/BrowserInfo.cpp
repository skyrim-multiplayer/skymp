#include "libespm/BrowserInfo.h"
#include "libespm/CombineBrowser.h"
#include "libespm/Utils.h"

namespace espm {

BrowserInfo::BrowserInfo(const CombineBrowser* parent_, uint8_t fileIdx_)
  : parent(parent_)
  , fileIdx(fileIdx_){};

uint32_t BrowserInfo::ToGlobalId(uint32_t rawId) const noexcept
{
  if (!parent) {
    return 0;
  }
  const auto mapping = parent->GetCombMapping(fileIdx);
  return utils::GetMappedId(rawId, *mapping);
}

}
