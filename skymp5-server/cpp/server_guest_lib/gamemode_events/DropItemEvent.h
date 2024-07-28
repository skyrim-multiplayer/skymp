#pragma once
#include "GameModeEvent.h"
#include <cstdint>

class DropItemEvent : public GameModeEvent
{
public:
  DropItemEvent(uint32_t refrId_, uint32_t baseId_, uint32_t count_)
    : refrId(refrId_)
    , baseId(baseId_)
    , count(count_)
  {
  }

  const char* GetName() const override { return "onDropItem"; }

  std::string GetArgumentsJsonArray() const override
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

private:
  uint32_t refrId = 0;
  uint32_t baseId = 0;
  uint32_t count = 0;
};
