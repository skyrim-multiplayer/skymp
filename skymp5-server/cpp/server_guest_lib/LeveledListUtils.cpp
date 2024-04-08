#include "LeveledListUtils.h"
#include <optional>
#include <random>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace {
bool IsLeveledType(const espm::LookupResult& lookupRes) noexcept
{
  espm::Type type = lookupRes.rec->GetType();
  return type == espm::LVLI::kType || type == espm::LVLN::kType ||
    type == "LVSP" /* for the future leveled spell implementation */;
}
}

std::vector<LeveledListUtils::Entry> LeveledListUtils::EvaluateList(
  const espm::CombineBrowser& br, const espm::LookupResult& lookupRes,
  uint32_t pcLevel, uint8_t* chanceNoneOverride)
{
  espm::CompressedFieldsCache dummyCache;

  const espm::LeveledListBase* leveledList = nullptr;
  if (IsLeveledType(lookupRes)) {
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
  if (IsLeveledType(lookupRes)) {
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
    if (IsLeveledType(eLookupRes)) {
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
  const espm::CombineBrowser& browser, const espm::LookupResult& headNpc,
  uint32_t pcLevel)
{
  std::vector<uint32_t> result;
  auto npcCursor = ConvertToNpc(headNpc);
  if (!npcCursor) {
    spdlog::error("EvaluateTemplateChain: NPC_ expected");
    return result;
  }

  uint32_t cursorFileIdx = headNpc.fileIdx;
  result.push_back(headNpc.ToGlobalId(headNpc.rec->GetId()));

  while (true) {
    uint32_t templateId = GetBaseTemplateId(npcCursor, cursorFileIdx, browser);
    if (templateId == 0) {
      break; // End if no base template
    }

    espm::LookupResult templateResult = browser.LookupById(templateId);

    if (!templateResult.rec) {
      spdlog::error("EvaluateTemplateChain: Not found template record");
      return result;
    }

    UpdateCursorAndResult(templateId, templateResult, npcCursor, cursorFileIdx,
                          result, browser, pcLevel);
  }

  return result;
}

const espm::NPC_* LeveledListUtils::ConvertToNpc(
  const espm::LookupResult& lookupResult)
{
  return espm::Convert<espm::NPC_>(lookupResult.rec);
}

uint32_t LeveledListUtils::GetBaseTemplateId(
  const espm::NPC_* cursor, uint32_t fileIdx,
  const espm::CombineBrowser& browser)
{
  espm::CompressedFieldsCache dummyCache;
  auto data = cursor->GetData(dummyCache);
  if (!data.baseTemplate) {
    return 0; // Indicates there's no base template
  }

  auto lookupRes = espm::LookupResult(&browser, cursor, fileIdx);
  return lookupRes.ToGlobalId(data.baseTemplate);
}

void LeveledListUtils::UpdateCursorAndResult(
  uint32_t templateId, espm::LookupResult& templateResult,
  const espm::NPC_*& cursor, uint32_t& cursorFileIdx,
  std::vector<uint32_t>& result, const espm::CombineBrowser& browser,
  uint32_t pcLevel)
{
  if (auto npc = espm::Convert<espm::NPC_>(templateResult.rec)) {
    result.push_back(templateId);
    cursor = npc;
    cursorFileIdx = templateResult.fileIdx;
  } else if (auto lvln = espm::Convert<espm::LVLN>(templateResult.rec)) {
    auto selectedNpcId =
      EvaluateAndSelectNpcId(browser, templateResult, pcLevel);
    if (selectedNpcId == 0) {
      return;
    }

    auto npcLookupRes = browser.LookupById(selectedNpcId);
    cursor = espm::Convert<espm::NPC_>(npcLookupRes.rec);
    result.push_back(selectedNpcId);
    cursorFileIdx = npcLookupRes.fileIdx;
  } else {
    spdlog::error("EvaluateTemplateChain: Not found template record");
  }
}

uint32_t LeveledListUtils::EvaluateAndSelectNpcId(
  const espm::CombineBrowser& browser,
  const espm::LookupResult& templateResult, uint32_t pcLevel)
{
  auto countByFormId =
    LeveledListUtils::EvaluateListRecurse(browser, templateResult, 1, pcLevel);

  if (countByFormId.empty()) {
    spdlog::error(
      "EvaluateTemplateChain: EvaluateListRecurse returned empty map");
    return 0;
  }

  if (countByFormId.size() > 1) {
    spdlog::warn("EvaluateTemplateChain: EvaluateListRecurse returned more "
                 "than 1 result, omitting other results");
  }

  // Return the id of the first npc in the map
  return countByFormId.begin()->first;
}
