#pragma once
#include "GameModeEvent.h"

class DeathEvent : public GameModeEvent
{
public:
  DeathEvent(uint32_t actorId_, uint32_t killerId_)
    : actorId(actorId_)
    , killerId(killerId_)
  {
  }

  const char* GetName() const override { return "onDeath"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(actorId);
    result += ",";
    result += std::to_string(killerId);
    result += "]";
    return result;
  }

private:
  uint32_t actorId = 0;
  uint32_t killerId = 0;
};
