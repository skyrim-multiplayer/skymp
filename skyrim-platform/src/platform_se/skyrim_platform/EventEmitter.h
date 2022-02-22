#pragma once

class EventEmitter
{
public:
  static void HitEvent(RE::TESHitEvent* event)
  {
    Emit<RE::TESHitEvent>(event);
  }

  static void MoveAttachDetachEvent(RE::TESMoveAttachDetachEvent* event)
  {
    Emit<RE::TESMoveAttachDetachEvent>(event);
  }

  static void ActivateEvent(RE::TESActivateEvent* event)
  {
    Emit<RE::TESActivateEvent>(event);
  }

  static void WaitStartEvent(RE::TESWaitStartEvent* event)
  {
    Emit<RE::TESWaitStartEvent>(event);
  }

  static void WaitStopEvent(RE::TESWaitStopEvent* event)
  {
    Emit<RE::TESWaitStopEvent>(event);
  }

  static void ObjectLoadedEvent(RE::TESObjectLoadedEvent* event)
  {
    Emit<RE::TESObjectLoadedEvent>(event);
  }

  static void LockChangedEvent(RE::TESLockChangedEvent* event)
  {
    Emit<RE::TESLockChangedEvent>(event);
  }

  static void CellFullyLoadedEvent(RE::TESCellFullyLoadedEvent* event)
  {
    Emit<RE::TESCellFullyLoadedEvent>(event);
  }

  static void CellAttachDetachEvent(RE::TESCellAttachDetachEvent* event)
  {
    Emit<RE::TESCellAttachDetachEvent>(event);
  }

  static void CellReadyToApplyDecalsEvent(
    RE::TESCellReadyToApplyDecalsEvent* event)
  {
    Emit<RE::TESCellReadyToApplyDecalsEvent>(event);
  }

  static void GrabReleaseEvent(RE::TESGrabReleaseEvent* event)
  {
    Emit<RE::TESGrabReleaseEvent>(event);
  }

  static void LoadGameEvent(RE::TESLoadGameEvent* event)
  {
    Emit<RE::TESLoadGameEvent>(event);
  }

  static void SwitchRaceCompleteEvent(RE::TESSwitchRaceCompleteEvent* event)
  {
    Emit<RE::TESSwitchRaceCompleteEvent>(event);
  }

  static void UniqueIDChangeEvent(RE::TESUniqueIDChangeEvent* event)
  {
    Emit<RE::TESUniqueIDChangeEvent>(event);
  }

  static void TrackedStatsEvent(RE::TESTrackedStatsEvent* event)
  {
    Emit<RE::TESTrackedStatsEvent>(event);
  }

  static void InitScriptEvent(RE::TESInitScriptEvent* event)
  {
    Emit<RE::TESInitScriptEvent>(event);
  }

  static void ResetEvent(RE::TESResetEvent* event)
  {
    Emit<RE::TESResetEvent>(event);
  }

  static void CombatEvent(RE::TESCombatEvent* event)
  {
    Emit<RE::TESCombatEvent>(event);
  }

  static void DeathEvent(RE::TESDeathEvent* event)
  {
    Emit<RE::TESDeathEvent>(event);
  }

  static void ContainerChangedEvent(RE::TESContainerChangedEvent* event)
  {
    Emit<RE::TESContainerChangedEvent>(event);
  }

  static void EquipEvent(RE::TESEquipEvent* event)
  {
    Emit<RE::TESEquipEvent>(event);
  }

  static void ActiveEffectApplyRemoveEvent(
    RE::TESActiveEffectApplyRemoveEvent* event)
  {
    Emit<RE::TESActiveEffectApplyRemoveEvent>(event);
  }

  static void MagicEffectApplyEvent(RE::TESMagicEffectApplyEvent* event)
  {
    Emit<RE::TESMagicEffectApplyEvent>(event);
  }

  static void OpenCloseEvent(RE::TESOpenCloseEvent* event)
  {
    Emit<RE::TESOpenCloseEvent>(event);
  }

  static void QuestInitEvent(RE::TESQuestInitEvent* event)
  {
    Emit<RE::TESQuestInitEvent>(event);
  }

  static void QuestStartStopEvent(RE::TESQuestStartStopEvent* event)
  {
    Emit<RE::TESQuestStartStopEvent>(event);
  }

  static void QuestStageEvent(RE::TESQuestStageEvent* event)
  {
    Emit<RE::TESQuestStageEvent>(event);
  }

  static void TriggerEvent(RE::TESTriggerEvent* event)
  {
    Emit<RE::TESTriggerEvent>(event);
  }

  static void TriggerEnterEvent(RE::TESTriggerEnterEvent* event)
  {
    Emit<RE::TESTriggerEnterEvent>(event);
  }

  static void TriggerLeaveEvent(RE::TESTriggerLeaveEvent* event)
  {
    Emit<RE::TESTriggerLeaveEvent>(event);
  }

  static void SleepStartEvent(RE::TESSleepStartEvent* event)
  {
    Emit<RE::TESSleepStartEvent>(event);
  }

  static void SleepStopEvent(RE::TESSleepStopEvent* event)
  {
    Emit<RE::TESSleepStopEvent>(event);
  }

  static void ActorLocationChangeEvent(RE::TESActorLocationChangeEvent* event)
  {
    Emit<RE::TESActorLocationChangeEvent>(event);
  }

  static void BookReadEvent(RE::TESBookReadEvent* event)
  {
    Emit<RE::TESBookReadEvent>(event);
  }

  static void SellEvent(RE::TESSellEvent* event)
  {
    Emit<RE::TESSellEvent>(event);
  }

  static void FurnitureEvent(RE::TESFurnitureEvent* event)
  {
    Emit<RE::TESFurnitureEvent>(event);
  }

  static void MagicWardHitEvent(RE::TESMagicWardHitEvent* event)
  {
    Emit<RE::TESMagicWardHitEvent>(event);
  }

  static void PackageEvent(RE::TESPackageEvent* event)
  {
    Emit<RE::TESPackageEvent>(event);
  }

  static void EnterBleedoutEvent(RE::TESEnterBleedoutEvent* event)
  {
    Emit<RE::TESEnterBleedoutEvent>(event);
  }

  static void DestructionStageChangedEvent(
    RE::TESDestructionStageChangedEvent* event)
  {
    Emit<RE::TESDestructionStageChangedEvent>(event);
  }

  static void SceneActionEvent(RE::TESSceneActionEvent* event)
  {
    Emit<RE::TESSceneActionEvent>(event);
  }

  static void SceneEvent(RE::TESSceneEvent* event)
  {
    Emit<RE::TESSceneEvent>(event);
  }

  static void PlayerBowShotEvent(RE::TESPlayerBowShotEvent* event)
  {
    Emit<RE::TESPlayerBowShotEvent>(event);
  }

  static void FastTravelEndEvent(RE::TESFastTravelEndEvent* event)
  {
    Emit<RE::TESFastTravelEndEvent>(event);
  }

  static void PerkEntryRunEvent(RE::TESPerkEntryRunEvent* event)
  {
    Emit<RE::TESPerkEntryRunEvent>(event);
  }

  static void ObjectREFRTranslationEvent(
    RE::TESObjectREFRTranslationEvent* event)
  {
    Emit<RE::TESObjectREFRTranslationEvent>(event);
  }

  static void ChestsLootedEvent() { RE::ChestsLooted::SendEvent(); }

  static void ItemsPickpocketedEvent(int32_t count)
  {
    RE::ItemsPickpocketed::SendEvent(count);
  }

  static void ActionEvent(SKSE::ActionEvent* event)
  {
    SKSE::GetActionEventSource()->SendEvent(event);
  }

  static void CameraEvent(SKSE::CameraEvent* event)
  {
    SKSE::GetCameraEventSource()->SendEvent(event);
  }

  static void CrosshairRefEvent(SKSE::CrosshairRefEvent* event)
  {
    SKSE::GetCrosshairRefEventSource()->SendEvent(event);
  }

  static void NiNodeUpdateEvent(SKSE::NiNodeUpdateEvent* event)
  {
    SKSE::GetNiNodeUpdateEventSource()->SendEvent(event);
  }

  static void ModCallbackEvent(SKSE::ModCallbackEvent* event)
  {
    SKSE::GetModCallbackEventSource()->SendEvent(event);
  }

private:
  template <class T>
  static void Emit(T* event)
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    holder->SendEvent<T>(event);
  }
};
