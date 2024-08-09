#pragma once
#include "GameModeEvent.h"

class RespawnEvent : public GameModeEvent
{
public:
  RespawnEvent(uint32_t actorId_)
    : actorId(actorId_)
  {
  }

  const char* GetName() const override { return "onRespawn"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(actorId);
    result += "]";
    return result;
  }

private:
  uint32_t actorId = 0;
};
