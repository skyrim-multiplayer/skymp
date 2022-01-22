#pragma once
#include "NullPointerException.h"

#include "TESEvents.h"
#include <RE/BGSFootstepEvent.h>
#include <RE/BGSFootstepManager.h>
#include <RE/BSInputDeviceManager.h>
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

#include <RE/ConsoleLog.h>

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
  , public RE::BSTEventSink<RE::InputEvent*>

{
public:
  static GameEventSinks* GetSingleton()
  {
    static GameEventSinks singleton;
    return &singleton;
  }

  enum class SinkClass
  {
    kNone,
    kTESActiveEffectApplyRemoveEvent,
    kTESLoadGameEvent,
    kTESEquipEvent,
    kTESHitEvent,
    kTESContainerChangedEvent,
    kTESDeathEvent,
    kTESMagicEffectApplyEvent,
    kTESCombatEvent,
    kTESResetEvent,
    kTESInitScriptEvent,
    kTESTrackedStatsEvent,
    kTESSwitchRaceCompleteEvent,
    kTESUniqueIDChangeEvent,
    kTESCellFullyLoadedEvent,
    kTESCellAttachDetachEvent,
    kTESGrabReleaseEvent,
    kTESLockChangedEvent,
    kTESMoveAttachDetachEvent,
    kTESObjectLoadedEvent,
    kTESWaitStartEvent,
    kTESWaitStopEvent,
    kTESActivateEvent,
    kMenuOpenCloseEvent,
    kTESSpellCastEvent,
    kTESOpenCloseEvent,
    kTESQuestInitEvent,
    kTESQuestStartStopEvent,
    kTESQuestStageEvent,
    kTESTriggerEvent,
    kTESTriggerEnterEvent,
    kTESTriggerLeaveEvent,
    kTESSleepStartEvent,
    kTESSleepStopEvent,
    kTESActorLocationChangeEvent,
    kTESBookReadEvent,
    kTESSellEvent,
    kTESFurnitureEvent,
    kTESMagicWardHitEvent,
    kTESPackageEvent,
    kTESEnterBleedoutEvent,
    kTESDestructionStageChangedEvent,
    kTESSceneActionEvent,
    kTESPlayerBowShotEvent,
    kTESFastTravelEndEvent,
    kTESObjectREFRTranslationEvent,
    kTESPerkEntryRunEvent,
    kSKSEActionEvent,
    kSKSECameraEvent,
    kSKSECrosshairRefEvent,
    kSKSENiNodeUpdateEvent,
    kSKSEModCallbackEvent,
    kPositionPlayerEvent,
    kBGSFootstepEvent,
    kInputEvent
  };

  enum class SinkAction
  {
    kAdd = 0,
    kRemove = 1
  };

  template <typename T>
  void ChangeEventSink(RE::BSTEventSource<T>* eventSource, SinkAction action)
  {
    if (action == SinkAction::kAdd) {
      eventSource->AddEventSink(dynamic_cast<RE::BSTEventSink<T>*>(this));
    }
    if (action == SinkAction::kRemove) {
      eventSource->RemoveEventSink(dynamic_cast<RE::BSTEventSink<T>*>(this));
    }
  }

  void EventSinksAction(SinkClass sink, SinkAction action)
  {
    auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (!scriptEventSource)
      throw NullPointerException("scriptEventSource");

    switch (sink) {
      case SinkClass::kNone: {
        break;
      }
      case SinkClass::kTESActiveEffectApplyRemoveEvent: {
        ChangeEventSink<RE::TESActivateEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESLoadGameEvent: {
        ChangeEventSink<RE::TESLoadGameEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESEquipEvent: {
        ChangeEventSink<RE::TESEquipEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESHitEvent: {
        ChangeEventSink<RE::TESHitEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESContainerChangedEvent: {
        ChangeEventSink<RE::TESContainerChangedEvent>(scriptEventSource,
                                                      action);
        break;
      }
      case SinkClass::kTESDeathEvent: {
        ChangeEventSink<RE::TESDeathEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESMagicEffectApplyEvent: {
        ChangeEventSink<RE::TESMagicEffectApplyEvent>(scriptEventSource,
                                                      action);
        break;
      }
      case SinkClass::kTESCombatEvent: {
        ChangeEventSink<RE::TESCombatEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESResetEvent: {
        ChangeEventSink<RE::TESResetEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESInitScriptEvent: {
        ChangeEventSink<RE::TESInitScriptEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESTrackedStatsEvent: {
        ChangeEventSink<RE::TESTrackedStatsEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESSwitchRaceCompleteEvent: {
        ChangeEventSink<RE::TESSwitchRaceCompleteEvent>(scriptEventSource,
                                                        action);
        break;
      }
      case SinkClass::kTESUniqueIDChangeEvent: {
        ChangeEventSink<RE::TESUniqueIDChangeEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESCellFullyLoadedEvent: {
        ChangeEventSink<RE::TESCellFullyLoadedEvent>(scriptEventSource,
                                                     action);
        break;
      }
      case SinkClass::kTESCellAttachDetachEvent: {
        ChangeEventSink<RE::TESCellAttachDetachEvent>(scriptEventSource,
                                                      action);
        break;
      }
      case SinkClass::kTESGrabReleaseEvent: {
        ChangeEventSink<RE::TESGrabReleaseEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESLockChangedEvent: {
        ChangeEventSink<RE::TESLockChangedEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESMoveAttachDetachEvent: {
        ChangeEventSink<RE::TESMoveAttachDetachEvent>(scriptEventSource,
                                                      action);
        break;
      }
      case SinkClass::kTESObjectLoadedEvent: {
        ChangeEventSink<RE::TESObjectLoadedEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESWaitStartEvent: {
        ChangeEventSink<RE::TESWaitStartEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESWaitStopEvent: {
        ChangeEventSink<RE::TESWaitStopEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESActivateEvent: {
        ChangeEventSink<RE::TESActivateEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kMenuOpenCloseEvent: {
        auto ui = RE::UI::GetSingleton();

        if (!ui) {
          throw NullPointerException("ui");
        }

        auto uiEventSource = ui->GetEventSource<RE::MenuOpenCloseEvent>();
        ChangeEventSink<RE::MenuOpenCloseEvent>(uiEventSource, action);
        break;
      }
      case SinkClass::kTESSpellCastEvent: {
        ChangeEventSink<RE::TESSpellCastEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESOpenCloseEvent: {
        ChangeEventSink<RE::TESOpenCloseEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESQuestInitEvent: {
        ChangeEventSink<RE::TESQuestInitEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESQuestStartStopEvent: {
        ChangeEventSink<RE::TESQuestStartStopEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESQuestStageEvent: {
        if (action == SinkAction::kAdd) {
          scriptEventSource->AddEventSink(
            dynamic_cast<RE::BSTEventSink<RE::TESQuestStageEvent>*>(this));
        }
        // WTF?
        // if (action == SinkAction::kRemove) {
        //   scriptEventSource->RemoveEventSink(
        //     dynamic_cast<RE::BSTEventSink<RE::TESQuestStageEvent>*>(this));
        // }
        break;
      }
      case SinkClass::kTESTriggerEvent: {
        ChangeEventSink<RE::TESTriggerEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESTriggerEnterEvent: {
        ChangeEventSink<RE::TESTriggerEnterEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESTriggerLeaveEvent: {
        ChangeEventSink<RE::TESTriggerLeaveEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESSleepStartEvent: {
        ChangeEventSink<RE::TESSleepStartEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESSleepStopEvent: {
        ChangeEventSink<RE::TESSleepStopEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESActorLocationChangeEvent: {
        ChangeEventSink<RE::TESActorLocationChangeEvent>(scriptEventSource,
                                                         action);
        break;
      }
      case SinkClass::kTESBookReadEvent: {
        ChangeEventSink<RE::TESBookReadEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESSellEvent: {
        ChangeEventSink<RE::TESSellEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESFurnitureEvent: {
        ChangeEventSink<RE::TESFurnitureEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESMagicWardHitEvent: {
        ChangeEventSink<RE::TESMagicWardHitEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESPackageEvent: {
        ChangeEventSink<RE::TESPackageEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESEnterBleedoutEvent: {
        ChangeEventSink<RE::TESEnterBleedoutEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESDestructionStageChangedEvent: {
        ChangeEventSink<RE::TESDestructionStageChangedEvent>(scriptEventSource,
                                                             action);
        break;
      }
      case SinkClass::kTESSceneActionEvent: {
        ChangeEventSink<RE::TESSceneActionEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESPlayerBowShotEvent: {
        ChangeEventSink<RE::TESPlayerBowShotEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESFastTravelEndEvent: {
        ChangeEventSink<RE::TESFastTravelEndEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kTESObjectREFRTranslationEvent: {
        ChangeEventSink<RE::TESObjectREFRTranslationEvent>(scriptEventSource,
                                                           action);
        break;
      }
      case SinkClass::kTESPerkEntryRunEvent: {
        ChangeEventSink<RE::TESPerkEntryRunEvent>(scriptEventSource, action);
        break;
      }
      case SinkClass::kSKSEActionEvent: {
        ChangeEventSink<SKSE::ActionEvent>(SKSE::GetActionEventSource(),
                                           action);
        break;
      }
      case SinkClass::kSKSECameraEvent: {
        ChangeEventSink<SKSE::CameraEvent>(SKSE::GetCameraEventSource(),
                                           action);
        break;
      }
      case SinkClass::kSKSECrosshairRefEvent: {
        ChangeEventSink<SKSE::CrosshairRefEvent>(
          SKSE::GetCrosshairRefEventSource(), action);
        break;
      }
      case SinkClass::kSKSENiNodeUpdateEvent: {
        ChangeEventSink<SKSE::NiNodeUpdateEvent>(
          SKSE::GetNiNodeUpdateEventSource(), action);
        break;
      }
      case SinkClass::kSKSEModCallbackEvent: {
        ChangeEventSink<SKSE::ModCallbackEvent>(
          SKSE::GetModCallbackEventSource(), action);
        break;
      }
      case SinkClass::kPositionPlayerEvent: {
        auto playerCharacterHolder = RE::PlayerCharacter::GetSingleton();
        auto playerCharacterEventSource = playerCharacterHolder
          ? static_cast<RE::BSTEventSource<RE::PositionPlayerEvent>*>(
              playerCharacterHolder)
          : throw NullPointerException("player character");

        ChangeEventSink<RE::PositionPlayerEvent>(playerCharacterEventSource,
                                                 action);
        break;
      }
      case SinkClass::kBGSFootstepEvent: {
        auto footstepHolder = RE::BGSFootstepManager::GetSingleton();
        auto footstepEventSource = footstepHolder
          ? static_cast<RE::BSTEventSource<RE::BGSFootstepEvent>*>(
              footstepHolder)
          : throw NullPointerException("footstep");

        ChangeEventSink<RE::BGSFootstepEvent>(footstepEventSource, action);
        break;
      }
      case SinkClass::kInputEvent: {
        auto inputHolder = RE::BSInputDeviceManager::GetSingleton();
        auto inputEventSource = inputHolder
          ? static_cast<RE::BSTEventSource<RE::InputEvent*>*>(inputHolder)
          : throw NullPointerException("inputHolder");

        ChangeEventSink<RE::InputEvent*>(inputEventSource, action);
        break;
      }
    }
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

  RE::BSEventNotifyControl ProcessEvent(
    RE::InputEvent* const* e,
    RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
};
