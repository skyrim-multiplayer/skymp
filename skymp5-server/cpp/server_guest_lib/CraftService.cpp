#include "CraftService.h"

#include "ConditionsEvaluator.h"
#include "MpActor.h"
#include "PartOne.h"
#include "RawMessageData.h"
#include "WorldState.h"
#include "gamemode_events/CraftEvent.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>
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

  MpActor* me = partOne.serverState.ActorByUser(rawMsgData.userId);
  if (!me) {
    return spdlog::error("Unable to craft without Actor attached");
  }

  auto workbenchBase = br.LookupById(workbench.GetBaseId());

  if (!workbenchBase.rec) {
    return spdlog::error("Workbench ref without base object {:x}",
                         workbench.GetFormId());
  }

  std::vector<uint32_t> workbenchKeywordIds =
    workbenchBase.rec->GetKeywordIds(cache);

  auto recipesList =
    FindRecipe(me, workbenchKeywordIds, br, inputObjects, resultObjectId);

  if (recipesList.empty()) {
    return spdlog::error(
      "Recipe not found: inputObjects={}, workbenchId={:#x}, "
      "resultObjectId={:#x}",
      inputObjects.ToJson().dump(), workbenchId, resultObjectId);
  }

  if (recipesList.size() > 1) {
    spdlog::warn("Found more than 1 recipe ({}), using the 1st one",
                 recipesList.size());
  }

  UseCraftRecipe(me, reinterpret_cast<const espm::COBJ*>(recipesList[0].rec),
                 cache, br, recipesList[0].fileIdx);
}

bool CraftService::RecipeItemsMatch(const espm::LookupResult& lookupRes,
                                    const Inventory& inputObjects,
                                    uint32_t resultObjectId)
{
  auto recipe = reinterpret_cast<const espm::COBJ*>(lookupRes.rec);

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

  auto thisInputObjects = recipeData.inputObjects;
  for (auto& entry : thisInputObjects) {
    auto formId = lookupRes.ToGlobalId(entry.formId);
    if (inputObjects.GetItemCount(formId) != entry.count) {
      return false;
    }
  }
  auto formId = lookupRes.ToGlobalId(recipeData.outputObjectFormId);
  if (formId != resultObjectId) {
    return false;
  }
  return true;
}

std::vector<espm::LookupResult> CraftService::FindRecipe(
  std::optional<MpActor*> me,
  std::optional<std::vector<uint32_t>> workbenchKeywordIds,
  const espm::CombineBrowser& br, const Inventory& inputObjects,
  uint32_t resultObjectId)
{
  if (allRecipes.empty()) {
    allRecipes = br.GetDistinctRecordsByType("COBJ");
  }

  std::vector<espm::LookupResult> candidatesConsideredUsable;

  for (auto& recipe : allRecipes) {
    if (!RecipeItemsMatch(recipe, inputObjects, resultObjectId)) {
      continue;
    }

    spdlog::info("CraftService::FindRecipe - Recipe candidate found: {:x}",
                 recipe.ToGlobalId(recipe.rec->GetId()));

    const bool canBeUsed =
      ConsiderRecipeCandidate(me, workbenchKeywordIds, recipe);
    if (canBeUsed) {
      candidatesConsideredUsable.push_back(recipe);
      spdlog::info("CraftService::FindRecipe - Recipe candidate usable");
    } else {
      spdlog::info("CraftService::FindRecipe - Recipe candidate not usable");
    }
  }

  return candidatesConsideredUsable;
}

bool CraftService::ConsiderRecipeCandidate(
  std::optional<MpActor*> me,
  std::optional<std::vector<uint32_t>> workbenchKeywordIds,
  const espm::LookupResult& lookupRes)
{
  auto cobj = reinterpret_cast<const espm::COBJ*>(lookupRes.rec);
  auto cobjData = cobj->GetData(cache);

  bool finalConsiderationResult = true;

  if (me.has_value()) {
    bool evalRes = EvaluateCraftRecipeConditions(*me, cobjData);
    if (!evalRes) {
      spdlog::info("CraftService::ConsiderRecipeCandidate - Craft recipe "
                   "conditions are not met");
      finalConsiderationResult = false;
    }
  } else {
    spdlog::info("CraftService::ConsiderRecipeCandidate - Actor not "
                 "specified, skipping conditions check");
  }

  if (workbenchKeywordIds.has_value()) {
    auto recipeBenchKeywordId = lookupRes.ToGlobalId(cobjData.benchKeywordId);

    // Note: In the original game, setting the benchmark keyword to NONE
    // removes the recipe from all crafting stations.

    bool includes =
      std::any_of(workbenchKeywordIds->begin(), workbenchKeywordIds->end(),
                  [&](uint32_t id) { return id == recipeBenchKeywordId; });

    if (!includes) {
      std::vector<std::string> hexIds;
      hexIds.reserve(workbenchKeywordIds->size());
      for (auto id : *workbenchKeywordIds) {
        hexIds.push_back(fmt::format("{:x}", id));
      }

      spdlog::info("CraftService::ConsiderRecipeCandidate - Craft recipe "
                   "workbench keywords don't match: recipe one {:x} is not in "
                   "workbench ids {}",
                   recipeBenchKeywordId, fmt::join(hexIds, ", "));
      finalConsiderationResult = false;
    }

  } else {
    spdlog::info("CraftService::ConsiderRecipeCandidate - Workbench keyword "
                 "id not specified, skipping bench keyword id check");
  }

  return finalConsiderationResult;
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
