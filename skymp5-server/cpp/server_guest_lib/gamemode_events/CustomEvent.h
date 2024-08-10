#pragma once
#include "GameModeEvent.h"

class CustomEvent : public GameModeEvent
{
public:
  CustomEvent(uint32_t refrId_, const std::string& eventName_,
              const std::string& argumentsJsonArrayDump_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState*) override;

  uint32_t refrId = 0;
  std::string eventName;
  std::string argumentsJsonArrayDump;
};
