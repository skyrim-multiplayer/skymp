#pragma once
#include "GameModeEvent.h"

class MpActor;

#include "Appearance.h"

class UpdateAppearanceAttemptEvent : public GameModeEvent
{
public:
  UpdateAppearanceAttemptEvent(MpActor* actor_, const Appearance& appearance_,
                               bool isAllowed_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;
  void OnFireBlocked(WorldState* worldState) override;

  MpActor* actor = nullptr;
  Appearance appearance;
  bool isAllowed = false;
};
