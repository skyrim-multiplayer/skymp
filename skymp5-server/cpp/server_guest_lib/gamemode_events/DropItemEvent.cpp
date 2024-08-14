#include "DropItemEvent.h"

DropItemEvent::DropItemEvent(uint32_t refrId_, uint32_t baseId_,
                             uint32_t count_)
  : refrId(refrId_)
  , baseId(baseId_)
  , count(count_)
{
}

const char* DropItemEvent::GetName() const
{
  return "onDropItem";
}

std::string DropItemEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(refrId);
  result += ",";
  result += std::to_string(baseId);
  result += ",";
  result += std::to_string(count);
  result += "]";
  return result;
}

void DropItemEvent::OnFireSuccess(WorldState*)
{
  // Do nothing. Handled in MpActor
}
