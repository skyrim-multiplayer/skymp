#pragma once
#include "GameModeEvent.h"

class ActivateEvent : public GameModeEvent
{
public:
  ActivateEvent(uint32_t refrId_, uint32_t casterRefrId_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  uint32_t refrId = 0;
  uint32_t casterRefrId = 0;
};
