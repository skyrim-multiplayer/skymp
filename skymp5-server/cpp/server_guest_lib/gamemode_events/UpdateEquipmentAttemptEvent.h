#pragma once
#include "GameModeEvent.h"

class MpActor;

#include "Equipment.h"

class UpdateEquipmentAttemptEvent : public GameModeEvent
{
public:
  UpdateEquipmentAttemptEvent(MpActor* actor_, const Equipment& equipment_,
                              bool isAllowed_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;
  void OnFireBlocked(WorldState* worldState) override;

  MpActor* actor = nullptr;
  Equipment equipment;
  bool isAllowed = false;
};
