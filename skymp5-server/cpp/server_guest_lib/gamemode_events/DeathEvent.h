#pragma once
#include "GameModeEvent.h"

class MpActor;

class DeathEvent : public GameModeEvent
{
public:
  DeathEvent(MpActor* actor_, MpActor* optionalKiller_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState*) override;

  MpActor* actor = nullptr;
  MpActor* optionalKiller = nullptr;
};
