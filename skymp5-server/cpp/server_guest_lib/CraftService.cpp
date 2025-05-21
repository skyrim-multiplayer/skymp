#include "CraftService.h"

#include "ConditionsEvaluator.h"
#include "MpActor.h"
#include "PartOne.h"
#include "RawMessageData.h"
#include "WorldState.h"
#include "gamemode_events/CraftEvent.h"
#include <algorithm>
#include <spdlog/spdlog.h>
#include <vector>

CraftService::CraftService(PartOne& partOne_)
  : partOne(partOne_)
{
}

void CraftService::OnCraftItem(const RawMessageData& rawMsgData,
                               const Inventory& inputObjects,
                               uint32_t workbenchId, uint32_t resultObjectId)
{
  auto& workbench =
    partOne.worldState.GetFormAt<MpObjectReference>(workbenchId);

  auto& br = partOne.worldState.GetEspm().GetBrowser();
  auto& cache = partOne.worldState.GetEspmCache();
  auto base = br.LookupById(workbench.GetBaseId());

  spdlog::info("User {} tries to craft {:#x} on workbench {:#x}",
               rawMsgData.userId, resultObjectId, workbenchId);

  bool isFurnitureOrActivator =
    base.rec->GetType() == "FURN" || base.rec->GetType() == "ACTI";
  if (!isFurnitureOrActivator) {
    return spdlog::error("Unable to use {} as workbench",
                         base.rec->GetType().ToString());
  }

  int espmIdx = 0;
  auto recipeUsed = FindRecipe(br, inputObjects, resultObjectId, &espmIdx);

  if (!recipeUsed) {
    return spdlog::error(
      "Recipe not found: inputObjects={}, workbenchId={:#x}, "
      "resultObjectId={:#x}",
      inputObjects.ToJson().dump(), workbenchId, resultObjectId);
  }

  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me) {
    return spdlog::error("Unable to craft without Actor attached");
  }

  bool evalRes = EvaluateCraftRecipeConditions(me, recipeUsed->GetData(cache));

  if (!evalRes) {
    return spdlog::error("Craft recipe conditions are not met");
  }

  UseCraftRecipe(me, recipeUsed, cache, br, espmIdx);
}

bool CraftService::RecipeMatches(const espm::IdMapping* mapping,
                                 const espm::COBJ* recipe,
                                 const Inventory& inputObjects,
                                 uint32_t resultObjectId)
{
  espm::CompressedFieldsCache dummyCache;
  auto recipeData = recipe->GetData(dummyCache);

  enum
  {
    ArmorTable = 0xadb78,
    SharpeningWheel = 0x88108
  };
  const bool isTemper = recipeData.benchKeywordId == ArmorTable ||
    recipeData.benchKeywordId == SharpeningWheel;
  if (isTemper) {
    return false;
  }

  // In the original game, setting the benchmark keyword to NONE removes the
  // recipe from all crafting stations.
  if (recipeData.benchKeywordId == 0) {
    return false;
  }

  auto thisInputObjects = recipeData.inputObjects;
  for (auto& entry : thisInputObjects) {
    auto formId = espm::utils::GetMappedId(entry.formId, *mapping);
    if (inputObjects.GetItemCount(formId) != entry.count) {
      return false;
    }
  }
  auto formId =
    espm::utils::GetMappedId(recipeData.outputObjectFormId, *mapping);
  if (formId != resultObjectId) {
    return false;
  }
  return true;
}

const espm::COBJ* CraftService::FindRecipe(const espm::CombineBrowser& br,
                                           const Inventory& inputObjects,
                                           uint32_t resultObjectId,
                                           int* optionalOutEspmIdx)
{
  // 1-st index is espm file index
  std::vector<const std::vector<const espm::RecordHeader*>*> allRecipes =
    br.GetRecordsByType("COBJ");

  const espm::COBJ* recipeUsed = nullptr;

  // multiple espm files can modify COBJ record. reverse order to find latest
  // record version first
  for (size_t i = allRecipes.size() - 1; i != static_cast<size_t>(-1); --i) {
    auto mapping = br.GetCombMapping(i);
    auto& espmLocalRecipes = allRecipes[i];
    auto it = std::find_if(
      espmLocalRecipes->begin(), espmLocalRecipes->end(),
      [&](const espm::RecordHeader* rec) {
        auto recipe = reinterpret_cast<const espm::COBJ*>(rec);
        return RecipeMatches(mapping, recipe, inputObjects, resultObjectId);
      });
    if (it != espmLocalRecipes->end()) {
      recipeUsed = reinterpret_cast<const espm::COBJ*>(*it);
      if (optionalOutEspmIdx)
        *optionalOutEspmIdx = static_cast<int>(i);
      break;
    }
  }
  return recipeUsed;
}

void CraftService::UseCraftRecipe(MpActor* me, const espm::COBJ* recipeUsed,
                                  espm::CompressedFieldsCache& cache,
                                  const espm::CombineBrowser& br, int espmIdx)
{
  auto recipeData = recipeUsed->GetData(cache);
  auto mapping = br.GetCombMapping(espmIdx);

  spdlog::info("Using craft recipe with EDID {} from espm file with index {}",
               recipeUsed->GetEditorId(cache), espmIdx);

  std::vector<Inventory::Entry> entries;
  for (auto& entry : recipeData.inputObjects) {
    auto formId = espm::utils::GetMappedId(entry.formId, *mapping);
    entries.push_back({ formId, entry.count });
  }

  auto outputFormId =
    espm::utils::GetMappedId(recipeData.outputObjectFormId, *mapping);

  if (spdlog::should_log(spdlog::level::info)) {
    std::string s = fmt::format("User formId={:#x} crafted", me->GetFormId());
    for (const auto& entry : entries) {
      s += fmt::format(" -{:#x} x{}", entry.baseId, entry.count);
    }
    s += fmt::format(" +{:#x} x{}", outputFormId, recipeData.outputCount);
    spdlog::info("{}", s);
  }

  auto recipeId = espm::utils::GetMappedId(recipeUsed->GetId(), *mapping);

  CraftEvent craftEvent(me, outputFormId, recipeData.outputCount, recipeId,
                        entries);

  craftEvent.Fire(me->GetParent());
}

bool CraftService::EvaluateCraftRecipeConditions(
  MpActor* me, const espm::COBJ::Data& recipeData)
{
  std::vector<Condition> conditions;
  std::transform(recipeData.conditions.begin(), recipeData.conditions.end(),
                 std::back_inserter(conditions),
                 [&](const auto& ctda) { return Condition::FromCtda(ctda); });

  // TODO: aggressor and target terms are not relevant for crafting
  const MpActor& aggressor = *me;
  const MpActor& target = *me;

  bool evalRes_ = false;

  auto callback = [&](bool evalRes, std::vector<std::string>& strings) {
    evalRes_ = evalRes;

    if (!strings.empty()) {
      if (evalRes) {
        strings.insert(strings.begin(),
                       fmt::format("EvaluateConditions result is true"));
      } else {
        strings.insert(strings.begin(),
                       fmt::format("EvaluateConditions result is false"));
      }
    }
  };

  static const ConditionsEvaluatorSettings kDefaultSettings;

  static const ConditionFunctionMap kEmptyMap;

  auto worldState = me->GetParent();

  const ConditionsEvaluatorSettings& settings =
    worldState ? worldState->conditionsEvaluatorSettings : kDefaultSettings;

  const ConditionFunctionMap& conditionFunctionMap =
    worldState ? worldState->conditionFunctionMap : kEmptyMap;

  ConditionsEvaluator::EvaluateConditions(
    conditionFunctionMap, settings, ConditionsEvaluatorCaller::kCraft,
    conditions, aggressor, target, callback);

  return evalRes_;
}
