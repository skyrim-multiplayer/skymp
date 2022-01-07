#include "TESEventEmitter.h"

template <class T>
void TESEventEmitter::Emit(T* event)
{
  auto sesh = RE::ScriptEventSourceHolder::GetSingleton();
  sesh->SendEvent<T>(event);
}

void TESEventEmitter::HitEvent(RE::TESHitEvent* event)
{
  Emit<RE::TESHitEvent>(event);
}

void TESEventEmitter::MoveAttachDetachEvent(
  RE::TESMoveAttachDetachEvent* event)
{
  Emit<RE::TESMoveAttachDetachEvent>(event);
}

void TESEventEmitter::ActivateEvent(RE::TESActivateEvent* event)
{
  Emit<RE::TESActivateEvent>(event);
}

void TESEventEmitter::WaitStopEvent(RE::TESWaitStopEvent* event)
{
  Emit<RE::TESWaitStopEvent>(event);
}

void TESEventEmitter::ObjectLoadedEvent(RE::TESObjectLoadedEvent* event)
{
  Emit<RE::TESObjectLoadedEvent>(event);
}

void TESEventEmitter::LockChangedEvent(RE::TESLockChangedEvent* event)
{
  Emit<RE::TESLockChangedEvent>(event);
}

void TESEventEmitter::CellFullyLoadedEvent(RE::TESCellFullyLoadedEvent* event)
{
  Emit<RE::TESCellFullyLoadedEvent>(event);
}

void TESEventEmitter::GrabReleaseEvent(RE::TESGrabReleaseEvent* event)
{
  Emit<RE::TESGrabReleaseEvent>(event);
}

void TESEventEmitter::LoadGameEvent(RE::TESLoadGameEvent* event)
{
  Emit<RE::TESLoadGameEvent>(event);
}

void TESEventEmitter::SwitchRaceCompleteEvent(
  RE::TESSwitchRaceCompleteEvent* event)
{
  Emit<RE::TESSwitchRaceCompleteEvent>(event);
}

void TESEventEmitter::UniqueIDChangeEvent(RE::TESUniqueIDChangeEvent* event)
{
  Emit<RE::TESUniqueIDChangeEvent>(event);
}

void TESEventEmitter::TrackedStatsEvent(RE::TESTrackedStatsEvent* event)
{
  Emit<RE::TESTrackedStatsEvent>(event);
}

void TESEventEmitter::InitScriptEvent(RE::TESInitScriptEvent* event)
{
  Emit<RE::TESInitScriptEvent>(event);
}

void TESEventEmitter::ResetEvent(RE::TESResetEvent* event)
{
  Emit<RE::TESResetEvent>(event);
}

void TESEventEmitter::CombatEvent(RE::TESCombatEvent* event)
{
  Emit<RE::TESCombatEvent>(event);
}

void TESEventEmitter::DeathEvent(RE::TESDeathEvent* event)
{
  Emit<RE::TESDeathEvent>(event);
}

void TESEventEmitter::ContainerChangedEvent(
  RE::TESContainerChangedEvent* event)
{
  Emit<RE::TESContainerChangedEvent>(event);
}

void TESEventEmitter::EquipEvent(RE::TESEquipEvent* event)
{
  Emit<RE::TESEquipEvent>(event);
}

void TESEventEmitter::ActiveEffectApplyRemoveEvent(
  RE::TESActiveEffectApplyRemoveEvent* event)
{
  Emit<RE::TESActiveEffectApplyRemoveEvent>(event);
}

void TESEventEmitter::MagicEffectApplyEvent(
  RE::TESMagicEffectApplyEvent* event)
{
  Emit<RE::TESMagicEffectApplyEvent>(event);
}
