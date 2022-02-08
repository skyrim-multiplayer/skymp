#pragma once

#include "EventHandlerBase.h"

class EventHandlerScript final
  : public EventHandlerBase
  , public RE::BSTEventSink<RE::TESActivateEvent>
  , public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
  , public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
  , public RE::BSTEventSink<RE::TESBookReadEvent>
  , public RE::BSTEventSink<RE::TESCellAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
  , public RE::BSTEventSink<RE::TESCombatEvent>
  , public RE::BSTEventSink<RE::TESContainerChangedEvent>
  , public RE::BSTEventSink<RE::TESDeathEvent>
  , public RE::BSTEventSink<RE::TESDestructionStageChangedEvent>
  , public RE::BSTEventSink<RE::TESEnterBleedoutEvent>
  , public RE::BSTEventSink<RE::TESEquipEvent>
  , public RE::BSTEventSink<RE::TESFastTravelEndEvent>
  , public RE::BSTEventSink<RE::TESFurnitureEvent>
  , public RE::BSTEventSink<RE::TESGrabReleaseEvent>
  , public RE::BSTEventSink<RE::TESHitEvent>
  , public RE::BSTEventSink<RE::TESInitScriptEvent>
  , public RE::BSTEventSink<RE::TESLoadGameEvent>
  , public RE::BSTEventSink<RE::TESLockChangedEvent>
  , public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
  , public RE::BSTEventSink<RE::TESMagicWardHitEvent>
  , public RE::BSTEventSink<RE::TESMoveAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESObjectLoadedEvent>
  , public RE::BSTEventSink<RE::TESObjectREFRTranslationEvent>
  , public RE::BSTEventSink<RE::TESOpenCloseEvent>
  , public RE::BSTEventSink<RE::TESPackageEvent>
  , public RE::BSTEventSink<RE::TESPerkEntryRunEvent>
  , public RE::BSTEventSink<RE::TESPlayerBowShotEvent>
  , public RE::BSTEventSink<RE::TESQuestInitEvent>
  , public RE::BSTEventSink<RE::TESQuestStageEvent>
  , public RE::BSTEventSink<RE::TESQuestStartStopEvent>
  , public RE::BSTEventSink<RE::TESResetEvent>
  , public RE::BSTEventSink<RE::TESSceneActionEvent>
  , public RE::BSTEventSink<RE::TESSellEvent>
  , public RE::BSTEventSink<RE::TESSleepStartEvent>
  , public RE::BSTEventSink<RE::TESSleepStopEvent>
  , public RE::BSTEventSink<RE::TESSpellCastEvent>
  , public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
  , public RE::BSTEventSink<RE::TESTrackedStatsEvent>
  , public RE::BSTEventSink<RE::TESTriggerEnterEvent>
  , public RE::BSTEventSink<RE::TESTriggerEvent>
  , public RE::BSTEventSink<RE::TESTriggerLeaveEvent>
  , public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>
  , public RE::BSTEventSink<RE::TESWaitStartEvent>
  , public RE::BSTEventSink<RE::TESWaitStopEvent>
{
public:
  [[nodiscard]] static EventHandlerScript* GetSingleton()
  {
    static EventHandlerScript singleton;
    return &singleton;
  }

  EventResult ProcessEvent(const RE::TESActivateEvent* event,
                           RE::BSTEventSource<RE::TESActivateEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESActiveEffectApplyRemoveEvent* event,
    RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESActorLocationChangeEvent* event,
    RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override;

  EventResult ProcessEvent(const RE::TESBookReadEvent* event,
                           RE::BSTEventSource<RE::TESBookReadEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESCellAttachDetachEvent* event,
    RE::BSTEventSource<RE::TESCellAttachDetachEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESCellFullyLoadedEvent* event,
    RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

  EventResult ProcessEvent(const RE::TESCombatEvent* event,
                           RE::BSTEventSource<RE::TESCombatEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESContainerChangedEvent* event,
    RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;

  EventResult ProcessEvent(const RE::TESDeathEvent* event,
                           RE::BSTEventSource<RE::TESDeathEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESDestructionStageChangedEvent* event,
    RE::BSTEventSource<RE::TESDestructionStageChangedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESEnterBleedoutEvent* event,
    RE::BSTEventSource<RE::TESEnterBleedoutEvent>*) override;

  EventResult ProcessEvent(const RE::TESEquipEvent* event,
                           RE::BSTEventSource<RE::TESEquipEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESFastTravelEndEvent* event,
    RE::BSTEventSource<RE::TESFastTravelEndEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESFurnitureEvent* event,
    RE::BSTEventSource<RE::TESFurnitureEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESGrabReleaseEvent* event,
    RE::BSTEventSource<RE::TESGrabReleaseEvent>*) override;

  EventResult ProcessEvent(const RE::TESHitEvent* event,
                           RE::BSTEventSource<RE::TESHitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESInitScriptEvent* event,
    RE::BSTEventSource<RE::TESInitScriptEvent>*) override;

  EventResult ProcessEvent(const RE::TESLoadGameEvent* event,
                           RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESLockChangedEvent* event,
    RE::BSTEventSource<RE::TESLockChangedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMagicEffectApplyEvent* event,
    RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMagicWardHitEvent* event,
    RE::BSTEventSource<RE::TESMagicWardHitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMoveAttachDetachEvent* event,
    RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESObjectLoadedEvent* event,
    RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESObjectREFRTranslationEvent* event,
    RE::BSTEventSource<RE::TESObjectREFRTranslationEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESOpenCloseEvent* event,
    RE::BSTEventSource<RE::TESOpenCloseEvent>*) override;

  EventResult ProcessEvent(const RE::TESPackageEvent* event,
                           RE::BSTEventSource<RE::TESPackageEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESPerkEntryRunEvent* event,
    RE::BSTEventSource<RE::TESPerkEntryRunEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESPlayerBowShotEvent* event,
    RE::BSTEventSource<RE::TESPlayerBowShotEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestInitEvent* event,
    RE::BSTEventSource<RE::TESQuestInitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestStageEvent* event,
    RE::BSTEventSource<RE::TESQuestStageEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESQuestStartStopEvent* event,
    RE::BSTEventSource<RE::TESQuestStartStopEvent>*) override;

  EventResult ProcessEvent(const RE::TESResetEvent* event,
                           RE::BSTEventSource<RE::TESResetEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSceneActionEvent* event,
    RE::BSTEventSource<RE::TESSceneActionEvent>*) override;

  EventResult ProcessEvent(const RE::TESSellEvent* event,
                           RE::BSTEventSource<RE::TESSellEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSleepStartEvent* event,
    RE::BSTEventSource<RE::TESSleepStartEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSleepStopEvent* event,
    RE::BSTEventSource<RE::TESSleepStopEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSpellCastEvent* event,
    RE::BSTEventSource<RE::TESSpellCastEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSwitchRaceCompleteEvent* event,
    RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTrackedStatsEvent* event,
    RE::BSTEventSource<RE::TESTrackedStatsEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTriggerEnterEvent* event,
    RE::BSTEventSource<RE::TESTriggerEnterEvent>*) override;

  EventResult ProcessEvent(const RE::TESTriggerEvent* event,
                           RE::BSTEventSource<RE::TESTriggerEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTriggerLeaveEvent* event,
    RE::BSTEventSource<RE::TESTriggerLeaveEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESUniqueIDChangeEvent* event,
    RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESWaitStartEvent* event,
    RE::BSTEventSource<RE::TESWaitStartEvent>*) override;

  EventResult ProcessEvent(const RE::TESWaitStopEvent* event,
                           RE::BSTEventSource<RE::TESWaitStopEvent>*) override;

private:
  EventHandlerScript()
  {
    AppendSink<RE::TESActivateEvent>(CreateEventList({ "activate" }));
    AppendSink<RE::TESActiveEffectApplyRemoveEvent>(
      CreateEventList({ "effectStart", "effectFinish" }));
    AppendSink<RE::TESActorLocationChangeEvent>(
      CreateEventList({ "locationChanged" }));
    AppendSink<RE::TESBookReadEvent>(CreateEventList({ "bookRead" }));
    AppendSink<RE::TESCellAttachDetachEvent>(
      CreateEventList({ "cellAttach", "cellDetach" }));
    AppendSink<RE::TESCellFullyLoadedEvent>(CreateEventList({ "cellFullyLoaded" }));
    AppendSink<RE::TESCombatEvent>(CreateEventList({ "combatState" }));
    AppendSink<RE::TESContainerChangedEvent>(CreateEventList({ "containerChanged" }));
    AppendSink<RE::TESDeathEvent>(CreateEventList({ "deathEnd", "deathStart" }));
    AppendSink<RE::TESDestructionStageChangedEvent>(
      CreateEventList({ "destructionStageChanged" }));
    AppendSink<RE::TESEnterBleedoutEvent>(CreateEventList({ "enterBleedout" }));
    AppendSink<RE::TESEquipEvent>(CreateEventList({ "equip", "unequip" }));
    AppendSink<RE::TESFastTravelEndEvent>(CreateEventList({ "fastTravelEnd" }));
    AppendSink<RE::TESFurnitureEvent>(
      CreateEventList({ "furnitureExit", "furnitureEnter" }));
    AppendSink<RE::TESGrabReleaseEvent>(CreateEventList({ "grabRelease" }));
    AppendSink<RE::TESHitEvent>(CreateEventList({ "hit" }));
    AppendSink<RE::TESInitScriptEvent>(CreateEventList({ "scriptInit" }));
    AppendSink<RE::TESLoadGameEvent>(CreateEventList({ "loadGame" }));
    AppendSink<RE::TESLockChangedEvent>(CreateEventList({ "lockChanged" }));
    AppendSink<RE::TESMagicEffectApplyEvent>(CreateEventList({ "magicEffectApply" }));
    AppendSink<RE::TESMagicWardHitEvent>(CreateEventList({ "wardHit" }));
    AppendSink<RE::TESMoveAttachDetachEvent>(CreateEventList({ "moveAttachDetach" }));
    AppendSink<RE::TESObjectLoadedEvent>(CreateEventList({ "objectLoaded" }));
    AppendSink<RE::TESObjectREFRTranslationEvent>(
      CreateEventList({ "translationFailed", "translationAlmostCompleted",
                 "translationCompleted" }));
    AppendSink<RE::TESOpenCloseEvent>(CreateEventList({ "open", "close" }));
    AppendSink<RE::TESPackageEvent>(
      CreateEventList({ "packageStart", "packageChange", "packageEnd" }));
    AppendSink<RE::TESPerkEntryRunEvent>(CreateEventList({ "perkEntryRun" }));
    AppendSink<RE::TESPlayerBowShotEvent>(CreateEventList({ "playerBowShot" }));
    AppendSink<RE::TESQuestInitEvent>(CreateEventList({ "questInit" }));
    AppendSink<RE::TESQuestStageEvent>(CreateEventList({ "questStage" }));
    AppendSink<RE::TESQuestStartStopEvent>(
      CreateEventList({ "questStart", "questStop" }));
    AppendSink<RE::TESResetEvent>(CreateEventList({ "reset" }));
    AppendSink<RE::TESSceneActionEvent>(CreateEventList({ "sceneAction" }));
    AppendSink<RE::TESSellEvent>(CreateEventList({ "sell" }));
    AppendSink<RE::TESSleepStartEvent>(CreateEventList({ "sleepStart" }));
    AppendSink<RE::TESSleepStopEvent>(CreateEventList({ "sleepStop" }));
    AppendSink<RE::TESSpellCastEvent>(CreateEventList({ "spellCast" }));
    AppendSink<RE::TESSwitchRaceCompleteEvent>(
      CreateEventList({ "switchRaceComplete" }));
    AppendSink<RE::TESTrackedStatsEvent>(CreateEventList({ "trackedStats" }));
    AppendSink<RE::TESTriggerEnterEvent>(CreateEventList({ "triggerEnter" }));
    AppendSink<RE::TESTriggerEvent>(CreateEventList({ "trigger" }));
    AppendSink<RE::TESTriggerLeaveEvent>(CreateEventList({ "triggerLeave" }));
    AppendSink<RE::TESUniqueIDChangeEvent>(CreateEventList({ "uniqueIdChange" }));
    AppendSink<RE::TESWaitStartEvent>(CreateEventList({ "waitStart" }));
    AppendSink<RE::TESWaitStopEvent>(CreateEventList({ "waitStop" }));
  };
  EventHandlerScript(const EventHandlerScript&) = delete;
  EventHandlerScript(EventHandlerScript&&) = delete;

  ~EventHandlerScript() = default;
};
