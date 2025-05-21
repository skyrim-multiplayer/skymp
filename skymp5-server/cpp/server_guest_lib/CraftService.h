#pragma once
#include "libespm/Loader.h"
#include <cstdint>
#include <vector>

class PartOne;
class Inventory;
struct RawMessageData;
class MpActor;

class CraftService
{
public:
  explicit CraftService(PartOne& partOne_);

  void OnCraftItem(const RawMessageData& rawMsgData,
                   const Inventory& inputObjects, uint32_t workbenchId,
                   uint32_t resultObjectId);

  // public for CraftTest.cpp
  bool RecipeMatches(const espm::IdMapping* mapping, const espm::COBJ* recipe,
                     const Inventory& inputObjects, uint32_t resultObjectId);

  // public for CraftTest.cpp
  const espm::COBJ* FindRecipe(const espm::CombineBrowser& br,
                               const Inventory& inputObjects,
                               uint32_t resultObjectId,
                               int* optionalOutEspmIdx);

private:
  void UseCraftRecipe(MpActor* me, const espm::COBJ* recipeUsed,
                      espm::CompressedFieldsCache& cache,
                      const espm::CombineBrowser& br, int espmIdx);

  bool EvaluateCraftRecipeConditions(MpActor* me,
                                     const espm::COBJ::Data& recipeData);

  PartOne& partOne;

  std::vector<espm::LookupResult> allRecipes;
};
