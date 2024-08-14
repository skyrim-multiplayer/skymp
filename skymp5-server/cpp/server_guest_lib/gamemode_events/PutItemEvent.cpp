#include "PutItemEvent.h"

#include "MpActor.h"

PutItemEvent::PutItemEvent(MpActor* actor_, MpObjectReference* sourceRefr_,
                           const Inventory::Entry& entry_)
  : actor(actor_)
  , sourceRefr(sourceRefr_)
  , entry(entry_)
{
}

const char* PutItemEvent::GetName() const
{
  return "onPutItem";
}

std::string PutItemEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(actor->GetFormId());
  result += ",";
  result += std::to_string(sourceRefr->GetFormId());
  result += ",";
  result += std::to_string(entry.baseId);
  result += ",";
  result += std::to_string(entry.count); // TODO: implement extra data
  result += "]";
  return result;
}

void PutItemEvent::OnFireSuccess(WorldState*)
{
  std::vector<Inventory::Entry> entries = { entry };
  actor->RemoveItems(entries, sourceRefr);
}
