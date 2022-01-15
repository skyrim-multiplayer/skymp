#include "TESEventEmitter.h"
#include <RE/ChestsLooted.h>
#include <RE/ItemsPickpocketed.h>

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

void TESEventEmitter::WaitStartEvent(TESEvents::TESWaitStartEvent* event)
{
  Emit<RE::TESWaitStartEvent>(reinterpret_cast<RE::TESWaitStartEvent*>(event));
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

void TESEventEmitter::CellAttachDetachEvent(
  TESEvents::TESCellAttachDetachEvent* event)
{
  Emit<RE::TESCellAttachDetachEvent>(
    reinterpret_cast<RE::TESCellAttachDetachEvent*>(event));
}

void TESEventEmitter::CellReadyToApplyDecalsEvent(
  TESEvents::TESCellReadyToApplyDecalsEvent* event)
{
  Emit<RE::TESCellReadyToApplyDecalsEvent>(
    reinterpret_cast<RE::TESCellReadyToApplyDecalsEvent*>(event));
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

void TESEventEmitter::OpenCloseEvent(TESEvents::TESOpenCloseEvent* event)
{
  Emit<RE::TESOpenCloseEvent>(reinterpret_cast<RE::TESOpenCloseEvent*>(event));
}

void TESEventEmitter::QuestInitEvent(TESEvents::TESQuestInitEvent* event)
{
  Emit<RE::TESQuestInitEvent>(reinterpret_cast<RE::TESQuestInitEvent*>(event));
}

void TESEventEmitter::QuestStartStopEvent(
  TESEvents::TESQuestStartStopEvent* event)
{
  Emit<RE::TESQuestStartStopEvent>(
    reinterpret_cast<RE::TESQuestStartStopEvent*>(event));
}

void TESEventEmitter::QuestStageEvent(TESEvents::TESQuestStageEvent* event)
{
  Emit<RE::TESQuestStageEvent>(
    reinterpret_cast<RE::TESQuestStageEvent*>(event));
}

void TESEventEmitter::TriggerEvent(TESEvents::TESTriggerEvent* event)
{
  Emit<RE::TESTriggerEvent>(reinterpret_cast<RE::TESTriggerEvent*>(event));
}

void TESEventEmitter::TriggerEnterEvent(TESEvents::TESTriggerEnterEvent* event)
{
  Emit<RE::TESTriggerEnterEvent>(
    reinterpret_cast<RE::TESTriggerEnterEvent*>(event));
}

void TESEventEmitter::TriggerLeaveEvent(TESEvents::TESTriggerLeaveEvent* event)
{
  Emit<RE::TESTriggerLeaveEvent>(
    reinterpret_cast<RE::TESTriggerLeaveEvent*>(event));
}

void TESEventEmitter::SleepStartEvent(TESEvents::TESSleepStartEvent* event)
{
  Emit<RE::TESSleepStartEvent>(
    reinterpret_cast<RE::TESSleepStartEvent*>(event));
}

void TESEventEmitter::SleepStopEvent(TESEvents::TESSleepStopEvent* event)
{
  Emit<RE::TESSleepStopEvent>(reinterpret_cast<RE::TESSleepStopEvent*>(event));
}

void TESEventEmitter::ActorLocationChangeEvent(
  TESEvents::TESActorLocationChangeEvent* event)
{
  Emit<RE::TESActorLocationChangeEvent>(
    reinterpret_cast<RE::TESActorLocationChangeEvent*>(event));
}

void TESEventEmitter::BookReadEvent(TESEvents::TESBookReadEvent* event)
{
  Emit<RE::TESBookReadEvent>(reinterpret_cast<RE::TESBookReadEvent*>(event));
}

void TESEventEmitter::SellEvent(TESEvents::TESSellEvent* event)
{
  Emit<RE::TESSellEvent>(reinterpret_cast<RE::TESSellEvent*>(event));
}

void TESEventEmitter::FurnitureEvent(TESEvents::TESFurnitureEvent* event)
{
  Emit<RE::TESFurnitureEvent>(reinterpret_cast<RE::TESFurnitureEvent*>(event));
}

void TESEventEmitter::MagicWardHitEvent(TESEvents::TESMagicWardHitEvent* event)
{
  Emit<RE::TESMagicWardHitEvent>(
    reinterpret_cast<RE::TESMagicWardHitEvent*>(event));
}

void TESEventEmitter::PackageEvent(TESEvents::TESPackageEvent* event)
{
  Emit<RE::TESPackageEvent>(reinterpret_cast<RE::TESPackageEvent*>(event));
}

void TESEventEmitter::EnterBleedoutEvent(
  TESEvents::TESEnterBleedoutEvent* event)
{
  Emit<RE::TESEnterBleedoutEvent>(
    reinterpret_cast<RE::TESEnterBleedoutEvent*>(event));
}

void TESEventEmitter::DestructionStageChangedEvent(
  TESEvents::TESDestructionStageChangedEvent* event)
{
  Emit<RE::TESDestructionStageChangedEvent>(
    reinterpret_cast<RE::TESDestructionStageChangedEvent*>(event));
}

void TESEventEmitter::SceneActionEvent(TESEvents::TESSceneActionEvent* event)
{
  Emit<RE::TESSceneActionEvent>(
    reinterpret_cast<RE::TESSceneActionEvent*>(event));
}

void TESEventEmitter::SceneEvent(TESEvents::TESSceneEvent* event)
{
  Emit<RE::TESSceneEvent>(reinterpret_cast<RE::TESSceneEvent*>(event));
}

void TESEventEmitter::PlayerBowShotEvent(
  TESEvents::TESPlayerBowShotEvent* event)
{
  Emit<RE::TESPlayerBowShotEvent>(
    reinterpret_cast<RE::TESPlayerBowShotEvent*>(event));
}

void TESEventEmitter::FastTravelEndEvent(
  TESEvents::TESFastTravelEndEvent* event)
{
  Emit<RE::TESFastTravelEndEvent>(
    reinterpret_cast<RE::TESFastTravelEndEvent*>(event));
}

void TESEventEmitter::PerkEntryRunEvent(TESEvents::TESPerkEntryRunEvent* event)
{
  Emit<RE::TESPerkEntryRunEvent>(
    reinterpret_cast<RE::TESPerkEntryRunEvent*>(event));
}

void TESEventEmitter::ObjectREFRTranslationEvent(
  TESEvents::TESObjectREFRTranslationEvent* event)
{
  Emit<RE::TESObjectREFRTranslationEvent>(
    reinterpret_cast<RE::TESObjectREFRTranslationEvent*>(event));
}

void TESEventEmitter::ChestsLootedEvent()
{
  RE::ChestsLooted::SendEvent();
}

void TESEventEmitter::ItemsPickpocketedEvent(SInt32 count)
{
  RE::ItemsPickpocketed::SendEvent(count);
}

void TESEventEmitter::ActionEvent(SKSE::ActionEvent* event)
{
  SKSE::GetActionEventSource()->SendEvent(event);
}

void TESEventEmitter::CameraEvent(SKSE::CameraEvent* event)
{
  SKSE::GetCameraEventSource()->SendEvent(event);
}

void TESEventEmitter::CrosshairRefEvent(SKSE::CrosshairRefEvent* event)
{
  SKSE::GetCrosshairRefEventSource()->SendEvent(event);
}

void TESEventEmitter::NiNodeUpdateEvent(SKSE::NiNodeUpdateEvent* event)
{
  SKSE::GetNiNodeUpdateEventSource()->SendEvent(event);
}

void TESEventEmitter::ModCallbackEvent(SKSE::ModCallbackEvent* event)
{
  SKSE::GetModCallbackEventSource()->SendEvent(event);
}
