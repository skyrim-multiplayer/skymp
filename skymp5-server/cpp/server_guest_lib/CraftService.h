#pragma once
#include "CraftItemMessage.h"
#include "MessageEvent.h"
#include "ServiceBase.h"

#include "libespm/Loader.h"
#include <cstdint>
#include <optional>
#include <vector>

class PartOne;
class Inventory;
struct RawMessageData;
class MpActor;

class CraftService : public ServiceBase<CraftService>
{
public:
  explicit CraftService(PartOne& partOne_);

  void OnCraftItem(const MessageEvent<CraftItemMessage> &event);

  // public for CraftTest.cpp
  bool RecipeItemsMatch(const espm::LookupResult& lookupRes,
                        const Inventory& inputObjects,
                        uint32_t resultObjectId);

  // public for CraftTest.cpp
  std::vector<espm::LookupResult> FindRecipe(
    std::optional<MpActor*> me,
    std::optional<std::vector<uint32_t>> workbenchKeywordIds,
    const espm::CombineBrowser& br, const Inventory& inputObjects,
    uint32_t resultObjectId);

private:
  bool ConsiderRecipeCandidate(
    std::optional<MpActor*> me,
    std::optional<std::vector<uint32_t>> workbenchKeywordIds,
    const espm::LookupResult& lookupRes);

  void UseCraftRecipe(MpActor* me, const espm::COBJ* recipeUsed,
                      espm::CompressedFieldsCache& cache,
                      const espm::CombineBrowser& br, int espmIdx);

  bool EvaluateCraftRecipeConditions(MpActor* me,
                                     const espm::COBJ::Data& recipeData);

  PartOne& partOne;
  std::vector<espm::LookupResult> allRecipes;
  espm::CompressedFieldsCache cache;
};
