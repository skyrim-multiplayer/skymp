#pragma once
#include "Inventory.h"
#include "Loader.h"
#include <cstdint>

bool RecipeMatches(const espm::IdMapping* mapping, const espm::COBJ* recipe,
                   const Inventory& inputObjects, uint32_t resultObjectId);

espm::COBJ* FindRecipe(const espm::CombineBrowser& br,
                       const Inventory& inputObjects, uint32_t resultObjectId,
                       int* optionalOutEspmIdx = nullptr);
