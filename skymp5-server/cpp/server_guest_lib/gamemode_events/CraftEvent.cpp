#include "CraftEvent.h"

#include "MpActor.h"

CraftEvent::CraftEvent(MpActor* actor_, uint32_t craftedItemBaseId_,
                       uint32_t count_, uint32_t recipeId_,
                       const std::vector<Inventory::Entry>& entries_)
  : actor(actor_)
  , craftedItemBaseId(craftedItemBaseId_)
  , count(count_)
  , recipeId(recipeId_)
  , entries(entries_)
{
}

const char* CraftEvent::GetName() const
{
  return "onCraft";
}

std::string CraftEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += std::to_string(craftedItemBaseId);
  result += ",";
  result += std::to_string(count);
  result += ",";
  result += std::to_string(recipeId);
  result += "]";
  return result;
}

void CraftEvent::OnFireSuccess(WorldState*)
{
  actor->RemoveItems(entries);
  actor->AddItem(craftedItemBaseId, count);
}
