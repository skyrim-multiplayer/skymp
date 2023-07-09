#pragma once
#include <cstdint>

namespace espm {

class CombineBrowser;

struct BrowserInfo
{
  BrowserInfo() = default;
  BrowserInfo(const CombineBrowser* parent_, uint8_t fileIdx_);

  // Returns 0 for empty (default constructed) LookupResult
  uint32_t ToGlobalId(uint32_t rawId) const noexcept;

  const CombineBrowser* const parent = nullptr;
  const uint8_t fileIdx = 0;
};

}
