#pragma once

#include "GameModeEvent.h"

#include <cstdint>

class MpActor;
class MpObjectReference;

class ItemPickupAttemptEvent : public GameModeEvent
{
public:
  ItemPickupAttemptEvent(MpObjectReference* sourceRefr_, MpActor* actor_,
                         uint32_t itemBaseId_, uint32_t itemCount_,
                         bool isOwned_, bool isQuestItem_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  MpObjectReference* sourceRefr = nullptr;
  MpActor* actor = nullptr;
  uint32_t itemBaseId = 0;
  uint32_t itemCount = 0;
  bool isOwned = false;
  bool isQuestItem = false;
};
