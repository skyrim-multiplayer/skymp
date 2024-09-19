#pragma once
#include "GameModeEvent.h"

class MpActor;
class MpObjectReference;

#include "Inventory.h"

class TakeItemEvent : public GameModeEvent
{
public:
  TakeItemEvent(MpActor* actor_, MpObjectReference* sourceRefr,
                const Inventory::Entry& entry_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

private:
  void OnFireSuccess(WorldState* worldState) override;

  MpActor* actor = nullptr;
  MpObjectReference* sourceRefr = nullptr;
  const Inventory::Entry& entry;
};
