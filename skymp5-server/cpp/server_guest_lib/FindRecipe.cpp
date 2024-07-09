#include "FindRecipe.h"
#include <algorithm>

bool RecipeMatches(const espm::IdMapping* mapping, const espm::COBJ* recipe,
                   const Inventory& inputObjects, uint32_t resultObjectId)
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

const espm::COBJ* FindRecipe(const espm::CombineBrowser& br,
                             const Inventory& inputObjects,
                             uint32_t resultObjectId, int* optionalOutEspmIdx)
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
