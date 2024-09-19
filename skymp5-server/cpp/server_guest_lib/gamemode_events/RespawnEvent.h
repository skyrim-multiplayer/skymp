#pragma once
#include "GameModeEvent.h"

class MpActor;

class RespawnEvent : public GameModeEvent
{
public:
  explicit RespawnEvent(MpActor* actor_, bool shouldTeleport_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

  void OnFireSuccess(WorldState*) override;

private:
  // event arguments
  MpActor* actor = nullptr;

  // OnFireSuccess-specific arguments
  bool shouldTeleport = false;
};
