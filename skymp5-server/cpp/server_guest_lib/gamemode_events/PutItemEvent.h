#pragma once
#include "GameModeEvent.h"

#include "Inventory.h"

class MpActor;
class MpObjectReference;

class PutItemEvent : public GameModeEvent
{
public:
  PutItemEvent(MpActor* actor_, MpObjectReference* sourceRefr_,
               const Inventory::Entry& entry_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  MpActor* actor = 0;
  MpObjectReference* sourceRefr = 0;
  const Inventory::Entry& entry; // splits to baseId and count
};
