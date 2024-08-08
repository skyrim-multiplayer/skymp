#pragma once
#include "GameModeEvent.h"

class CraftEvent : public GameModeEvent
{
public:
  CraftEvent(uint32_t actorId_, uint32_t craftedItemBaseId_, uint32_t count_,
             uint32_t recipeId_)
    : actorId(actorId_)
    , craftedItemBaseId(craftedItemBaseId_)
    , count(count_)
    , recipeId(recipeId_)
  {
  }

  const char* GetName() const override { return "onCraft"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(actorId);
    result += ",";
    result += std::to_string(craftedItemBaseId);
    result += ",";
    result += std::to_string(count);
    result += ",";
    result += std::to_string(recipeId);
    result += "]";
    return result;
  }

private:
  uint32_t actorId = 0;
  uint32_t craftedItemBaseId = 0;
  uint32_t count = 0;
  uint32_t recipeId = 0;
};
