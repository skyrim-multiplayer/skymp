#pragma once
#include "NullPointerException.h"

#include "TESEvents.h"
#include <RE/BGSFootstepEvent.h>
#include <RE/BGSFootstepManager.h>
#include <RE/MenuOpenCloseEvent.h>
#include <RE/PlayerCharacter.h>
#include <RE/PositionPlayerEvent.h>
#include <RE/ScriptEventSourceHolder.h>
#include <RE/TESActiveEffectApplyRemoveEvent.h>
#include <RE/TESCellFullyLoadedEvent.h>
#include <RE/TESCombatEvent.h>
#include <RE/TESContainerChangedEvent.h>
#include <RE/TESDeathEvent.h>
#include <RE/TESEquipEvent.h>
#include <RE/TESGrabReleaseEvent.h>
#include <RE/TESHitEvent.h>
#include <RE/TESInitScriptEvent.h>
#include <RE/TESLockChangedEvent.h>
#include <RE/TESMagicEffectApplyEvent.h>
#include <RE/TESMoveAttachDetachEvent.h>
#include <RE/TESObjectLoadedEvent.h>
#include <RE/TESResetEvent.h>
#include <RE/TESSwitchRaceCompleteEvent.h>
#include <RE/TESTrackedStatsEvent.h>
#include <RE/TESUniqueIDChangeEvent.h>
#include <RE/TESWaitStopEvent.h>
#include <RE/UI.h>
#include <SKSE/API.h>

class TaskQueue;

class GameEventSinks
  : public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
  , public RE::BSTEventSink<RE::TESLoadGameEvent>
  , public RE::BSTEventSink<RE::TESEquipEvent>
  , public RE::BSTEventSink<RE::TESHitEvent>
  , public RE::BSTEventSink<RE::TESContainerChangedEvent>
  , public RE::BSTEventSink<RE::TESDeathEvent>
  , public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
  , public RE::BSTEventSink<RE::TESCombatEvent>
  , public RE::BSTEventSink<RE::TESResetEvent>
  , public RE::BSTEventSink<RE::TESInitScriptEvent>
  , public RE::BSTEventSink<RE::TESTrackedStatsEvent>
  , public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
  , public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>
  , public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
  , public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESGrabReleaseEvent>
  , public RE::BSTEventSink<RE::TESLockChangedEvent>
  , public RE::BSTEventSink<RE::TESMoveAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESObjectLoadedEvent>
  , public RE::BSTEventSink<RE::TESWaitStartEvent>
  , public RE::BSTEventSink<RE::TESWaitStopEvent>
  , public RE::BSTEventSink<RE::TESActivateEvent>
  , public RE::BSTEventSink<RE::MenuOpenCloseEvent>
  , public RE::BSTEventSink<RE::TESSpellCastEvent>
  , public RE::BSTEventSink<RE::TESOpenCloseEvent>
  , public RE::BSTEventSink<RE::TESQuestInitEvent>
  , public RE::BSTEventSink<RE::TESQuestStartStopEvent>
  , public RE::BSTEventSink<RE::TESQuestStageEvent>
  , public RE::BSTEventSink<RE::TESTriggerEvent>
  , public RE::BSTEventSink<RE::TESTriggerEnterEvent>
  , public RE::BSTEventSink<RE::TESTriggerLeaveEvent>
  , public RE::BSTEventSink<RE::TESSleepStartEvent>
  , public RE::BSTEventSink<RE::TESSleepStopEvent>
  , public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
  , public RE::BSTEventSink<RE::TESBookReadEvent>
  , public RE::BSTEventSink<RE::TESSellEvent>
  , public RE::BSTEventSink<RE::TESFurnitureEvent>
  , public RE::BSTEventSink<RE::TESMagicWardHitEvent>
  , public RE::BSTEventSink<RE::TESPackageEvent>
  , public RE::BSTEventSink<RE::TESEnterBleedoutEvent>
  , public RE::BSTEventSink<RE::TESDestructionStageChangedEvent>
  , public RE::BSTEventSink<RE::TESSceneActionEvent>
  , public RE::BSTEventSink<RE::TESPlayerBowShotEvent>
  , public RE::BSTEventSink<RE::TESFastTravelEndEvent>
  , public RE::BSTEventSink<RE::TESObjectREFRTranslationEvent>
  , public RE::BSTEventSink<RE::TESPerkEntryRunEvent>
  , public RE::BSTEventSink<SKSE::ActionEvent>
  , public RE::BSTEventSink<SKSE::CameraEvent>
  , public RE::BSTEventSink<SKSE::CrosshairRefEvent>
  , public RE::BSTEventSink<SKSE::NiNodeUpdateEvent>
  , public RE::BSTEventSink<SKSE::ModCallbackEvent>
  , public RE::BSTEventSink<RE::PositionPlayerEvent>
  , public RE::BSTEventSink<RE::BGSFootstepEvent>

{
public:
  GameEventSinks()
  {
    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (!holder)
      throw NullPointerException("holder");

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESActivateEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESMoveAttachDetachEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESHitEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESEquipEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESLoadGameEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESDeathEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESContainerChangedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESMagicEffectApplyEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESResetEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESInitScriptEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESCombatEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESTrackedStatsEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESUniqueIDChangeEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESGrabReleaseEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESCellFullyLoadedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESCellAttachDetachEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESLockChangedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESObjectLoadedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESWaitStartEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESWaitStopEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>*>(
        this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSpellCastEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESOpenCloseEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESQuestInitEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESQuestStartStopEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESQuestStageEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESTriggerEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESTriggerEnterEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESTriggerLeaveEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSleepStartEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSleepStopEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESActorLocationChangeEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESBookReadEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSellEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESFurnitureEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESMagicWardHitEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESPackageEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESEnterBleedoutEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESDestructionStageChangedEvent>*>(
        this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESSceneActionEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESPlayerBowShotEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESFastTravelEndEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESObjectREFRTranslationEvent>*>(
        this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESPerkEntryRunEvent>*>(this));

    auto ui = RE::UI::GetSingleton();

    if (!ui) {
      throw NullPointerException("ui");
    }

    ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::MenuOpenCloseEvent>*>(this));

    SKSE::GetActionEventSource()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<SKSE::ActionEvent>*>(this));

    SKSE::GetCameraEventSource()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<SKSE::CameraEvent>*>(this));

    SKSE::GetCrosshairRefEventSource()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<SKSE::CrosshairRefEvent>*>(this));

    SKSE::GetNiNodeUpdateEventSource()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<SKSE::NiNodeUpdateEvent>*>(this));

    SKSE::GetModCallbackEventSource()->AddEventSink(
      dynamic_cast<RE::BSTEventSink<SKSE::ModCallbackEvent>*>(this));

    auto playerCharacterHolder = RE::PlayerCharacter::GetSingleton();
    auto playerCharacterEventSource = playerCharacterHolder
      ? static_cast<RE::BSTEventSource<RE::PositionPlayerEvent>*>(
          playerCharacterHolder)
      : throw NullPointerException("player character");

    playerCharacterEventSource->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::PositionPlayerEvent>*>(this));

    auto footstepHolder = RE::BGSFootstepManager::GetSingleton();
    auto footstepEventSource = footstepHolder
      ? static_cast<RE::BSTEventSource<RE::BGSFootstepEvent>*>(footstepHolder)
      : throw NullPointerException("footstep");

    footstepEventSource->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::BGSFootstepEvent>*>(this));
  }

private:
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESActivateEvent* event_,
    RE::BSTEventSource<RE::TESActivateEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESMoveAttachDetachEvent* event_,
    RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESWaitStartEvent* event_,
    RE::BSTEventSource<RE::TESWaitStartEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESWaitStopEvent* event_,
    RE::BSTEventSource<RE::TESWaitStopEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESObjectLoadedEvent* event_,
    RE::BSTEventSource<RE::TESObjectLoadedEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLockChangedEvent* event_,
    RE::BSTEventSource<RE::TESLockChangedEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESCellFullyLoadedEvent* event_,
    RE::BSTEventSource<RE::TESCellFullyLoadedEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESCellAttachDetachEvent* event_,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESGrabReleaseEvent* event_,
    RE::BSTEventSource<RE::TESGrabReleaseEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESLoadGameEvent* event_,
    RE::BSTEventSource<RE::TESLoadGameEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSwitchRaceCompleteEvent* event_,
    RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESUniqueIDChangeEvent* event_,
    RE::BSTEventSource<RE::TESUniqueIDChangeEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESTrackedStatsEvent* event_,
    RE::BSTEventSource<RE::TESTrackedStatsEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESInitScriptEvent* event_,
    RE::BSTEventSource<RE::TESInitScriptEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESResetEvent* event_,
    RE::BSTEventSource<RE::TESResetEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESCombatEvent* event_,
    RE::BSTEventSource<RE::TESCombatEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESDeathEvent* event_,
    RE::BSTEventSource<RE::TESDeathEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESContainerChangedEvent* event_,
    RE::BSTEventSource<RE::TESContainerChangedEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESHitEvent* event_,
    RE::BSTEventSource<RE::TESHitEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESEquipEvent* event_,
    RE::BSTEventSource<RE::TESEquipEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESActiveEffectApplyRemoveEvent* event_,
    RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>* eventSource)
    override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event_,
    RE::BSTEventSource<RE::TESMagicEffectApplyEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::MenuOpenCloseEvent* e,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSpellCastEvent* e,
    RE::BSTEventSource<RE::TESSpellCastEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESOpenCloseEvent* e,
    RE::BSTEventSource<RE::TESOpenCloseEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESQuestInitEvent* e,
    RE::BSTEventSource<RE::TESQuestInitEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESQuestStartStopEvent* e,
    RE::BSTEventSource<RE::TESQuestStartStopEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESQuestStageEvent* e,
    RE::BSTEventSource<RE::TESQuestStageEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESTriggerEvent* e,
    RE::BSTEventSource<RE::TESTriggerEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESTriggerEnterEvent* e,
    RE::BSTEventSource<RE::TESTriggerEnterEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESTriggerLeaveEvent* e,
    RE::BSTEventSource<RE::TESTriggerLeaveEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSleepStartEvent* e,
    RE::BSTEventSource<RE::TESSleepStartEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSleepStopEvent* e,
    RE::BSTEventSource<RE::TESSleepStopEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESActorLocationChangeEvent* e,
    RE::BSTEventSource<RE::TESActorLocationChangeEvent>* a_eventSource)
    override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESBookReadEvent* e,
    RE::BSTEventSource<RE::TESBookReadEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSellEvent* e,
    RE::BSTEventSource<RE::TESSellEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESFurnitureEvent* e,
    RE::BSTEventSource<RE::TESFurnitureEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESMagicWardHitEvent* e,
    RE::BSTEventSource<RE::TESMagicWardHitEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESPackageEvent* e,
    RE::BSTEventSource<RE::TESPackageEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESEnterBleedoutEvent* e,
    RE::BSTEventSource<RE::TESEnterBleedoutEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESDestructionStageChangedEvent* e,
    RE::BSTEventSource<RE::TESDestructionStageChangedEvent>* a_eventSource)
    override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESSceneActionEvent* e,
    RE::BSTEventSource<RE::TESSceneActionEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESPlayerBowShotEvent* e,
    RE::BSTEventSource<RE::TESPlayerBowShotEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESFastTravelEndEvent* e,
    RE::BSTEventSource<RE::TESFastTravelEndEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESObjectREFRTranslationEvent* e,
    RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>* a_eventSource)
    override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESPerkEntryRunEvent* e,
    RE::BSTEventSource<RE::TESPerkEntryRunEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const SKSE::ActionEvent* e,
    RE::BSTEventSource<SKSE::ActionEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const SKSE::CameraEvent* e,
    RE::BSTEventSource<SKSE::CameraEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const SKSE::CrosshairRefEvent* e,
    RE::BSTEventSource<SKSE::CrosshairRefEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const SKSE::NiNodeUpdateEvent* e,
    RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const SKSE::ModCallbackEvent* e,
    RE::BSTEventSource<SKSE::ModCallbackEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::PositionPlayerEvent* e,
    RE::BSTEventSource<RE::PositionPlayerEvent>* a_eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::BGSFootstepEvent* e,
    RE::BSTEventSource<RE::BGSFootstepEvent>* a_eventSource) override;
};
