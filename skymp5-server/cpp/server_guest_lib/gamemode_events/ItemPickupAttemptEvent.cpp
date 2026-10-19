#include "ItemPickupAttemptEvent.h"

#include "MpActor.h"
#include "MpObjectReference.h"

ItemPickupAttemptEvent::ItemPickupAttemptEvent(
  MpObjectReference* sourceRefr_, MpActor* actor_, uint32_t itemBaseId_,
  uint32_t itemCount_, bool isOwned_, bool isQuestItem_)
  : sourceRefr(sourceRefr_)
  , actor(actor_)
  , itemBaseId(itemBaseId_)
  , itemCount(itemCount_)
  , isOwned(isOwned_)
  , isQuestItem(isQuestItem_)
{
}

const char* ItemPickupAttemptEvent::GetName() const
{
  return "onItemPickupAttempt";
}

std::string ItemPickupAttemptEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(sourceRefr->GetFormId());
  result += ",";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += std::to_string(itemBaseId);
  result += ",";
  result += std::to_string(itemCount);
  result += ",";
  result += isOwned ? "true" : "false";
  result += ",";
  result += isQuestItem ? "true" : "false";
  result += "]";
  return result;
}

void ItemPickupAttemptEvent::OnFireSuccess(WorldState*)
{
}
