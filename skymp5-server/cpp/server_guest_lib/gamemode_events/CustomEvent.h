#pragma once
#include "GameModeEvent.h"

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
    return argumentsJsonArrayDump;
  }

private:
  uint32_t refrId = 0;
  std::string eventName;
  std::string argumentsJsonArrayDump;
};
