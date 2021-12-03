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

  auto thisInputObjects = recipeData.inputObjects;
  for (auto& entry : thisInputObjects) {
    auto formId = espm::GetMappedId(entry.formId, *mapping);
    if (inputObjects.GetItemCount(formId) != entry.count) {
      return false;
    }
  }
  auto formId = espm::GetMappedId(recipeData.outputObjectFormId, *mapping);
  if (formId != resultObjectId) {
    return false;
  }
  return true;
}

espm::COBJ* FindRecipe(const espm::CombineBrowser& br,
                       const Inventory& inputObjects, uint32_t resultObjectId,
                       int* optionalOutEspmIdx)
{
  auto allRecipes = br.GetRecordsByType("COBJ");

  espm::COBJ* recipeUsed = nullptr;

  for (size_t i = 0; i < allRecipes.size(); ++i) {
    auto mapping = br.GetCombMapping(i);
    auto& espmLocalRecipes = allRecipes[i];
    auto it = std::find_if(espmLocalRecipes->begin(), espmLocalRecipes->end(),
                           [&](espm::RecordHeader* rec) {
                             auto recipe = reinterpret_cast<espm::COBJ*>(rec);
                             return RecipeMatches(
                               mapping, recipe, inputObjects, resultObjectId);
                           });
    if (it != espmLocalRecipes->end()) {
      recipeUsed = reinterpret_cast<espm::COBJ*>(*it);
      if (optionalOutEspmIdx)
        *optionalOutEspmIdx = static_cast<int>(i);
      break;
    }
  }
  return recipeUsed;
}
