#pragma once
#include <cstdint>
#include <espm.h>
#include <map>
#include <vector>

#include <Combiner.h>

namespace LeveledListUtils {
struct Entry
{
  uint32_t formId = 0;
  uint32_t count = 0;
};

std::vector<Entry> EvaluateList(const espm::CombineBrowser& br,
                                const espm::LookupResult& lookupRes,
                                uint32_t pcLevel = 0,
                                uint8_t* chanceNoneOverride = nullptr);

std::map<uint32_t, uint32_t> EvaluateListRecurse(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t countMult = 1, uint32_t pcLevel = 0,
  uint8_t* chanceNoneOverride = nullptr);
}
