#include "ActivateEvent.h"

ActivateEvent::ActivateEvent(uint32_t refrId_, uint32_t casterRefrId_)
  : refrId(refrId_)
  , casterRefrId(casterRefrId_)
{
}

const char* ActivateEvent::GetName() const
{
  return "onActivate";
}

std::string ActivateEvent::GetArgumentsJsonArray() const
{
  std::string result;
  result += "[";
  result += std::to_string(refrId);
  result += ",";
  result += std::to_string(casterRefrId);
  result += "]";
  return result;
}

void ActivateEvent::OnFireSuccess(WorldState*)
{
  // Do nothing. Handled in MpObjectReference
}
