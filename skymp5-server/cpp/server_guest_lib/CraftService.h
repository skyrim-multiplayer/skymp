#pragma once
#include <cstdint>
#include "libespm/Loader.h"

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

private:
  bool RecipeMatches(const espm::IdMapping* mapping, const espm::COBJ* recipe,
                     const Inventory& inputObjects, uint32_t resultObjectId);

  const espm::COBJ* FindRecipe(const espm::CombineBrowser& br,
                               const Inventory& inputObjects,
                               uint32_t resultObjectId,
                               int* optionalOutEspmIdx);

  void UseCraftRecipe(MpActor* me, const espm::COBJ* recipeUsed,
                      espm::CompressedFieldsCache& cache,
                      const espm::CombineBrowser& br, int espmIdx);

  bool EvaluateCraftRecipeConditions(MpActor* me,
                                     const espm::COBJ::Data& recipeData);

  PartOne& partOne;
};
