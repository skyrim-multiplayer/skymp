#pragma once
#include "TESEvents.h"
#include <RE/ScriptEventSourceHolder.h>
#include <SKSE/API.h>

class TESEventEmitter
{
public:
  static void HitEvent(RE::TESHitEvent* event);
  static void MoveAttachDetachEvent(RE::TESMoveAttachDetachEvent* event);
  static void ActivateEvent(RE::TESActivateEvent* event);
  static void WaitStartEvent(TESEvents::TESWaitStartEvent* event);
  static void WaitStopEvent(RE::TESWaitStopEvent* event);
  static void ObjectLoadedEvent(RE::TESObjectLoadedEvent* event);
  static void LockChangedEvent(RE::TESLockChangedEvent* event);
  static void CellFullyLoadedEvent(RE::TESCellFullyLoadedEvent* event);
  static void CellAttachDetachEvent(
    TESEvents::TESCellAttachDetachEvent* event);
  static void CellReadyToApplyDecalsEvent(
    TESEvents::TESCellReadyToApplyDecalsEvent* event);
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
  static void OpenCloseEvent(TESEvents::TESOpenCloseEvent* event);
  static void QuestInitEvent(TESEvents::TESQuestInitEvent* event);
  static void QuestStartStopEvent(TESEvents::TESQuestStartStopEvent* event);
  static void QuestStageEvent(TESEvents::TESQuestStageEvent* event);
  static void TriggerEvent(TESEvents::TESTriggerEvent* event);
  static void TriggerEnterEvent(TESEvents::TESTriggerEnterEvent* event);
  static void TriggerLeaveEvent(TESEvents::TESTriggerLeaveEvent* event);
  static void SleepStartEvent(TESEvents::TESSleepStartEvent* event);
  static void SleepStopEvent(TESEvents::TESSleepStopEvent* event);
  static void ActorLocationChangeEvent(
    TESEvents::TESActorLocationChangeEvent* event);
  static void BookReadEvent(TESEvents::TESBookReadEvent* event);
  static void SellEvent(TESEvents::TESSellEvent* event);
  static void FurnitureEvent(TESEvents::TESFurnitureEvent* event);
  static void MagicWardHitEvent(TESEvents::TESMagicWardHitEvent* event);
  static void PackageEvent(TESEvents::TESPackageEvent* event);
  static void EnterBleedoutEvent(TESEvents::TESEnterBleedoutEvent* event);
  static void DestructionStageChangedEvent(
    TESEvents::TESDestructionStageChangedEvent* event);
  static void SceneActionEvent(TESEvents::TESSceneActionEvent* event);
  static void SceneEvent(TESEvents::TESSceneEvent* event);
  static void PlayerBowShotEvent(TESEvents::TESPlayerBowShotEvent* event);
  static void FastTravelEndEvent(TESEvents::TESFastTravelEndEvent* event);
  static void PerkEntryRunEvent(TESEvents::TESPerkEntryRunEvent* event);
  static void ObjectREFRTranslationEvent(
    TESEvents::TESObjectREFRTranslationEvent* event);
  static void ChestsLootedEvent();
  static void ItemsPickpocketedEvent(SInt32 count);
  static void ActionEvent(SKSE::ActionEvent* event);
  static void CameraEvent(SKSE::CameraEvent* event);
  static void CrosshairRefEvent(SKSE::CrosshairRefEvent* event);
  static void NiNodeUpdateEvent(SKSE::NiNodeUpdateEvent* event);
  static void ModCallbackEvent(SKSE::ModCallbackEvent* event);

private:
  template <class T>
  static void Emit(T* event);
};
