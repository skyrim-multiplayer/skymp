#include "LeveledListUtils.h"
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <optional>

std::vector<LeveledListUtils::Entry> LeveledListUtils::EvaluateList(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t pcLevel, uint8_t* chanceNoneOverride)
{
  espm::CompressedFieldsCache dummyCache;

  const espm::LeveledListBase* leveledList = nullptr;
  if (lookupRes.rec->GetType() == espm::LVLI::kType ||
      lookupRes.rec->GetType() == espm::LVLN::kType ||
      lookupRes.rec->GetType() ==
        "LVSP") /* for the future leveled spell implementation */ {
    leveledList =
      reinterpret_cast<const espm::LeveledListBase*>(lookupRes.rec);
  }
  if (!leveledList) {
    return {};
  }
  auto data = leveledList->GetData(dummyCache);

  std::vector<Entry> res;

  int chanceNone = data.chanceNoneGlobalId ? 100 : data.chanceNone;
  if (chanceNoneOverride) {
    chanceNone = *chanceNoneOverride;
  }

  std::uniform_real_distribution<double> dist(0.0, 100.0);

  std::random_device rd;
  std::mt19937 mt(rd());
  bool none = dist(mt) < chanceNone;
  if (!none) {
    std::vector<const espm::LeveledListBase::Entry*> entriesAllowed;
    for (size_t i = 0; i < data.numEntries; ++i) {
      if (!pcLevel || data.entries[i].level <= pcLevel) {
        entriesAllowed.push_back(&data.entries[i]);
      }
    }

    bool useRandomEntry =
      !(data.leveledItemFlags & espm::LeveledListBase::UseAll);
    if (useRandomEntry && !entriesAllowed.empty()) {
      std::uniform_int_distribution<int> dist(0, entriesAllowed.size() - 1);
      auto entry = entriesAllowed[dist(mt)];

      uint32_t formId = lookupRes.ToGlobalId(entry->formId);
      res.push_back({ formId, entry->count });
    } else {
      for (auto entry : entriesAllowed) {
        uint32_t formId = lookupRes.ToGlobalId(entry->formId);
        res.push_back({ formId, entry->count });
      }
    }
  }
  return res;
}

std::map<uint32_t, uint32_t> LeveledListUtils::EvaluateListRecurse(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t countMult, uint32_t pcLevel, uint8_t* chanceNoneOverride)
{
  espm::CompressedFieldsCache dummyCache;

  const espm::LeveledListBase* leveledList = nullptr;
  if (lookupRes.rec->GetType() == espm::LVLI::kType ||
      lookupRes.rec->GetType() == espm::LVLN::kType ||
      lookupRes.rec->GetType() ==
        "LVSP") /* for the future leveled spell implementation */ {
    leveledList =
      reinterpret_cast<const espm::LeveledListBase*>(lookupRes.rec);
  }

  bool calcForEach = leveledList &&
    (leveledList->GetData(dummyCache).leveledItemFlags &
     espm::LeveledListBase::Each);

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
  auto firstEvalRes = EvaluateList(br, lookupRes, pcLevel, chanceNoneOverride);
  for (auto& e : firstEvalRes) {
    auto eLookupRes = br.LookupById(e.formId);
    if (!eLookupRes.rec) {
      continue;
    }
    if (eLookupRes.rec->GetType() == espm::LVLI::kType ||
        eLookupRes.rec->GetType() == espm::LVLN::kType ||
        eLookupRes.rec->GetType() ==
          "LVSP") /* for the future leveled spell implementation */ {
      auto childRes = EvaluateListRecurse(br, eLookupRes, 1, pcLevel);
      for (auto& p : childRes) {
        res[p.first] += p.second;
      }

    } else {
      res[e.formId] += e.count;
    }
  }

  if (countMult != 1 && !calcForEach) {
    for (auto& p : res) {
      p.second *= countMult;
    }
  }

  return res;
}

std::vector<uint32_t> LeveledListUtils::EvaluateTemplateChain(
  const espm::CombineBrowser& br, const espm::LookupResult& headNpc,
  uint32_t pcLevel)
{
  espm::CompressedFieldsCache dummyCache;
  std::vector<uint32_t> res;

  auto cursor = espm::Convert<espm::NPC_>(headNpc.rec);
  auto cursorFileIdx = headNpc.fileIdx;
  const auto cursorToGlobalId = [&](uint32_t rawId) {
    auto lookupRes = espm::LookupResult(&br, cursor, cursorFileIdx);
    return lookupRes.ToGlobalId(rawId);
  };
  if (!cursor) {
    spdlog::error("EvaluateTemplateChain: NPC_ expected");
    return res;
  }

  res.push_back(headNpc.ToGlobalId(headNpc.rec->GetId()));

  while (1) {
    espm::NPC_::Data data = cursor->GetData(dummyCache);
    if (!data.baseTemplate) {
      break;
    }

    uint32_t templateId = cursorToGlobalId(data.baseTemplate);
    espm::LookupResult template_ = br.LookupById(templateId);

    if (!template_.rec) {
      spdlog::error("EvaluateTemplateChain: Not found template record");
      return res;
    }

    if (auto npc = espm::Convert<espm::NPC_>(template_.rec)) {
      res.push_back(templateId);
      cursor = npc;
      cursorFileIdx = template_.fileIdx;
    } else if (auto lvln = espm::Convert<espm::LVLN>(template_.rec)) {
      std::map<uint32_t, uint32_t> countByFormId =
        EvaluateListRecurse(br, template_, 1, pcLevel);
      if (countByFormId.empty()) {
        spdlog::error(
          "EvaluateTemplateChain: EvaluateListRecurse returned empty map");
        return res;
      }
      if (countByFormId.size() > 1) {
        spdlog::warn("EvaluateTemplateChain: EvaluateListRecurse returned "
                     "more than 1 result, omitting other results");
      }
      auto [templateIdNpc, _] = *countByFormId.begin();
      auto npcLookupRes = br.LookupById(templateIdNpc);
      auto npc = espm::Convert<espm::NPC_>(npcLookupRes.rec);

      res.push_back(templateIdNpc);
      cursor = npc;
      cursorFileIdx = npcLookupRes.fileIdx;
    } else {
      spdlog::error("EvaluateTemplateChain: Not found template record");
      return res;
    }
  }

  return res;
}
