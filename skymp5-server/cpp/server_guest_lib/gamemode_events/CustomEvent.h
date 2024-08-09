#pragma once
#include "GameModeEvent.h"
#include <nlohmann/json.hpp>

class CustomEvent : public GameModeEvent
{
public:
  explicit CustomEvent(uint32_t refrId_, const std::string& eventName_,
                       const std::string& argumentsJsonArrayDump_)
    : refrId(refrId_)
    , eventName(eventName_)
    , argumentsJsonArrayDump(argumentsJsonArrayDump_)
  {
  }

  const char* GetName() const override { return eventName.c_str(); }

  std::string GetArgumentsJsonArray() const override
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

private:
  uint32_t refrId = 0;
  std::string eventName;
  std::string argumentsJsonArrayDump;
};
