#pragma once
#include "GameModeEvent.h"

class MpActor;

class DeathEvent : public GameModeEvent
{
public:
  DeathEvent(MpActor* actor_, MpActor* optionalKiller_,
             float healthPercentageBeforeDeath_,
             float magickaPercentageBeforeDeath_,
             float staminaPercentageBeforeDeath_);

  const char* GetName() const override;

  std::string GetArgumentsJsonArray() const override;

  uint32_t GetDyingActorId() const;
  float GetHealthPercentageBeforeDeath() const noexcept;
  float GetMagickaPercentageBeforeDeath() const noexcept;
  float GetStaminaPercentageBeforeDeath() const noexcept;

private:
  void OnFireSuccess(WorldState*) override;

  MpActor* actor = nullptr;
  MpActor* optionalKiller = nullptr;
  float healthPercentageBeforeDeath = 0.f;
  float magickaPercentageBeforeDeath = 0.f;
  float staminaPercentageBeforeDeath = 0.f;
};
