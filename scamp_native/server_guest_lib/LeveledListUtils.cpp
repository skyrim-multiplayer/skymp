#include "LeveledListUtils.h"
#include <random>

std::vector<LeveledListUtils::Entry> LeveledListUtils::EvaluateList(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t pcLevel)
{
  auto leveledList = espm::Convert<espm::LVLI>(lookupRes.rec);
  if (!leveledList)
    return {};
  auto data = leveledList->GetData();

  std::vector<Entry> res;

  int chanceNone = data.chanceNoneGlobalId ? 100 : data.chanceNone;

  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_real_distribution<double> dist(0.0, 100.0);

  bool none = dist(mt) < chanceNone;
  if (!none) {

    std::uniform_int_distribution<int> dist(0, data.numEntries - 1);

    int idx = (data.leveledItemFlags & espm::LVLI::UseAll) ? -1 : dist(mt);

    while (idx != -1 && pcLevel && data.entries[idx].level > pcLevel)
      idx = dist(mt);

    for (size_t i = 0; i < data.numEntries; ++i) {
      if (pcLevel && data.entries[i].level > pcLevel)
        continue;

      if (idx != -1 && idx != i)
        continue;

      uint32_t formId = lookupRes.ToGlobalId(data.entries[i].formId);

      res.push_back({ formId, data.entries[i].count });
    }
  }
  return res;
}

std::map<uint32_t, uint32_t> LeveledListUtils::EvaluateListRecurse(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t countMult, uint32_t pcLevel)
{
  auto leveledList = espm::Convert<espm::LVLI>(lookupRes.rec);
  bool calcForEach = leveledList &&
    (leveledList->GetData().leveledItemFlags & espm::LVLI::Each);

  if (calcForEach && countMult != 1) {
    std::map<uint32_t, uint32_t> res;
    for (uint32_t i = 0; i < countMult; ++i) {
      auto stepRes = EvaluateListRecurse(br, lookupRes, 1, pcLevel);
      for (auto& p : stepRes) {
        res[p.first] += p.second;
      }
    }
    return res;
  }

  std::map<uint32_t, uint32_t> res;
  auto firstEvalRes = EvaluateList(br, lookupRes, pcLevel);
  for (auto& e : firstEvalRes) {
    auto eLookupRes = br.LookupById(e.formId);
    if (!eLookupRes.rec)
      continue;
    if (eLookupRes.rec->GetType() == espm::LVLI::type) {
      auto childRes = EvaluateListRecurse(br, eLookupRes, 1, pcLevel);
      for (auto& p : childRes) {
        res[p.first] += p.second;
      }

    } else {
      res[e.formId] += e.count;
    }
  }

  if (countMult != 1 && !calcForEach) {
    for (auto& p : res)
      p.second *= countMult;
  }

  return res;
}