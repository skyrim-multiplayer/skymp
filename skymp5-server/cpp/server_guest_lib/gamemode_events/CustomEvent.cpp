#include "CustomEvent.h"

#include <nlohmann/json.hpp>

CustomEvent::CustomEvent(uint32_t refrId_, const std::string& eventName_,
                         const std::string& argumentsJsonArrayDump_)
  : refrId(refrId_)
  , eventName(eventName_)
  , argumentsJsonArrayDump(argumentsJsonArrayDump_)
{
}

const char* CustomEvent::GetName() const
{
  return eventName.c_str();
}

std::string CustomEvent::GetArgumentsJsonArray() const
{
  const nlohmann::json argumentsJsonArray =
    nlohmann::json::parse(argumentsJsonArrayDump);

  auto newArray = nlohmann::json::array();
  newArray.push_back(refrId);

  for (const auto& element : argumentsJsonArray) {
    newArray.push_back(element);
  }

  return newArray.dump();
}

void CustomEvent::OnFireSuccess(WorldState*)
{
  // Do nothing
}
