#pragma once
#include "TESEvents.h"
#include <RE/ScriptEventSourceHolder.h>

class TESEventEmitter
{
public:
  static void HitEvent(RE::TESHitEvent* event);
  static void MoveAttachDetachEvent(RE::TESMoveAttachDetachEvent* event);
  static void ActivateEvent(RE::TESActivateEvent* event);
  static void WaitStopEvent(RE::TESWaitStopEvent* event);
  static void ObjectLoadedEvent(RE::TESObjectLoadedEvent* event);
  static void LockChangedEvent(RE::TESLockChangedEvent* event);
  static void CellFullyLoadedEvent(RE::TESCellFullyLoadedEvent* event);
  static void GrabReleaseEvent(RE::TESGrabReleaseEvent* event);
  static void LoadGameEvent(RE::TESLoadGameEvent* event);
  static void SwitchRaceCompleteEvent(RE::TESSwitchRaceCompleteEvent* event);
  static void UniqueIDChangeEvent(RE::TESUniqueIDChangeEvent* event);
  static void TrackedStatsEvent(RE::TESTrackedStatsEvent* event);
  static void InitScriptEvent(RE::TESInitScriptEvent* event);
  static void ResetEvent(RE::TESResetEvent* event);
  static void CombatEvent(RE::TESCombatEvent* event);
  static void DeathEvent(RE::TESDeathEvent* event);
  static void ContainerChangedEvent(RE::TESContainerChangedEvent* event);
  static void EquipEvent(RE::TESEquipEvent* event);
  static void ActiveEffectApplyRemoveEvent(
    RE::TESActiveEffectApplyRemoveEvent* event);
  static void MagicEffectApplyEvent(RE::TESMagicEffectApplyEvent* event);

  static void TestEvent();

private:
  template <class T>
  static void Emit(T* event);
};
