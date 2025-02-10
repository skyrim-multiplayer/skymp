#include "EatItemEvent.h"

#include "MpActor.h"
#include "WorldState.h"
#include <unordered_set>
#include <vector>

EatItemEvent::EatItemEvent(MpActor* actor_, uint32_t baseId_,
                           bool isIngredient_, bool isAlchemyItem_)
  : actor(actor_)
  , baseId(baseId_)
  , isIngredient(isIngredient_)
  , isAlchemyItem(isAlchemyItem_)
{
}

const char* EatItemEvent::GetName() const
{
  return "onEatItem";
}

std::string EatItemEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += std::to_string(baseId);
  result += "]";
  return result;
}

void EatItemEvent::OnFireSuccess(WorldState* worldState)
{
  std::vector<espm::Effects::Effect> effects;
  if (isAlchemyItem) {
    effects = espm::GetData<espm::ALCH>(baseId, worldState).effects;
  } else if (isIngredient) {
    effects = espm::GetData<espm::INGR>(baseId, worldState).effects;
  } else {
    return;
  }
  std::unordered_set<std::string> modFiles = { worldState->espmFiles.begin(),
                                               worldState->espmFiles.end() };
  bool hasSweetpie = modFiles.count("SweetPie.esp");
  actor->ApplyMagicEffects(effects, hasSweetpie);
}
