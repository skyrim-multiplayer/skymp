#pragma once
#include "GameModeEvent.h"
#include <cstdint>

class DropItemEvent : public GameModeEvent
{
public:
  DropItemEvent(uint32_t refrId_, uint32_t baseId_, uint32_t count_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  uint32_t refrId = 0;
  uint32_t baseId = 0;
  uint32_t count = 0;
};
