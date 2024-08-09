#pragma once
#include "GameModeEvent.h"

class TakeItemEvent : public GameModeEvent
{
public:
  TakeItemEvent(uint32_t actorId_, uint32_t sourceRefrId, uint32_t baseId_,
                uint32_t count_)
    : actorId(actorId_)
    , sourceRefrId(sourceRefrId)
    , baseId(baseId_)
    , count(count_)
  {
  }

  const char* GetName() const override { return "onTakeItem"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(actorId);
    result += ",";
    result += std::to_string(sourceRefrId);
    result += ",";
    result += std::to_string(baseId);
    result += ",";
    result += std::to_string(count);
    result += "]";
    return result;
  }

private:
  uint32_t actorId = 0;
  uint32_t sourceRefrId = 0;
  uint32_t baseId = 0;
  uint32_t count = 0;
};
