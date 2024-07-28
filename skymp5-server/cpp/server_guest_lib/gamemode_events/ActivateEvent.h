#pragma once
#include "GameModeEvent.h"

class ActivateEvent : public GameModeEvent
{
public:
  ActivateEvent(uint32_t refrId_, uint32_t casterRefrId_)
    : refrId(refrId_)
    , casterRefrId(casterRefrId_)
  {
  }

  const char* GetName() const override { return "onActivate"; }

  std::string GetArgumentsJsonArray() const override
  {
    std::string result;
    result += "[";
    result += std::to_string(refrId);
    result += ",";
    result += std::to_string(casterRefrId);
    result += "]";
    return result;
  }

private:
  uint32_t refrId = 0;
  uint32_t casterRefrId = 0;
};
