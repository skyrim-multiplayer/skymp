#pragma once
#include "GameModeEvent.h"

class EatItemEvent : public GameModeEvent
{
public:
  EatItemEvent(uint32_t refrId_, uint32_t baseId_)
    : refrId(refrId_)
    , baseId(baseId_)
  {
  }

  const char* GetName() const override { return "onEatItem"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(refrId);
    result += ",";
    result += std::to_string(baseId);
    result += "]";
    return result;
  }

private:
  uint32_t refrId = 0;
  uint32_t baseId = 0;
};
