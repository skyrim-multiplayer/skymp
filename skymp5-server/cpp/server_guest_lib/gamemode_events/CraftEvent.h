#pragma once
#include "GameModeEvent.h"

#include "Inventory.h"

class MpActor;

class CraftEvent : public GameModeEvent
{
public:
  CraftEvent(MpActor* actor_, uint32_t craftedItemBaseId_, uint32_t count_,
             uint32_t recipeId_,
             const std::vector<Inventory::Entry>& entries_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  // event arguments
  MpActor* actor = 0;
  uint32_t craftedItemBaseId = 0;
  uint32_t count = 0;
  uint32_t recipeId = 0;

  // OnFireSuccess-specific arguments
  const std::vector<Inventory::Entry>& entries;
};
