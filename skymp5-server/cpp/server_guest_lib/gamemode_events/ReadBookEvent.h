#pragma once
#include "GameModeEvent.h"

class MpActor;

class ReadBookEvent : public GameModeEvent
{
public:
  ReadBookEvent(MpActor* actor_, uint32_t baseId_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

  bool SpellLearned() const;

private:
  void OnFireSuccess(WorldState* worldState) override;

  // event arguments
  MpActor* actor = nullptr;
  uint32_t baseId = 0;

  // OnFireSuccess-specific arguments
  // ...

  // Results
  bool spellLearned = false;
};
