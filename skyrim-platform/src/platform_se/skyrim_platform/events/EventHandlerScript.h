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
    AppendSink<RE::TESActivateEvent>(CreateEV({ "activate" }));
    AppendSink<RE::TESActiveEffectApplyRemoveEvent>(
      CreateEV({ "effectStart", "effectFinish" }));
    AppendSink<RE::TESActorLocationChangeEvent>(
      CreateEV({ "locationChanged" }));
    AppendSink<RE::TESBookReadEvent>(CreateEV({ "bookRead" }));
    AppendSink<RE::TESCellAttachDetachEvent>(
      CreateEV({ "cellAttach", "cellDetach" }));
    AppendSink<RE::TESCellFullyLoadedEvent>(CreateEV({ "cellFullyLoaded" }));
    AppendSink<RE::TESCombatEvent>(CreateEV({ "combatState" }));
    AppendSink<RE::TESContainerChangedEvent>(CreateEV({ "containerChanged" }));
    AppendSink<RE::TESDeathEvent>(CreateEV({ "deathEnd", "deathStart" }));
    AppendSink<RE::TESDestructionStageChangedEvent>(
      CreateEV({ "destructionStageChanged" }));
    AppendSink<RE::TESEnterBleedoutEvent>(CreateEV({ "enterBleedout" }));
    AppendSink<RE::TESEquipEvent>(CreateEV({ "equip", "unequip" }));
    AppendSink<RE::TESFastTravelEndEvent>(CreateEV({ "fastTravelEnd" }));
    AppendSink<RE::TESFurnitureEvent>(
      CreateEV({ "furnitureExit", "furnitureEnter" }));
    AppendSink<RE::TESGrabReleaseEvent>(CreateEV({ "grabRelease" }));
    AppendSink<RE::TESHitEvent>(CreateEV({ "hit" }));
    AppendSink<RE::TESInitScriptEvent>(CreateEV({ "scriptInit" }));
    AppendSink<RE::TESLoadGameEvent>(CreateEV({ "loadGame" }));
    AppendSink<RE::TESLockChangedEvent>(CreateEV({ "lockChanged" }));
    AppendSink<RE::TESMagicEffectApplyEvent>(CreateEV({ "magicEffectApply" }));
    AppendSink<RE::TESMagicWardHitEvent>(CreateEV({ "wardHit" }));
    AppendSink<RE::TESMoveAttachDetachEvent>(CreateEV({ "moveAttachDetach" }));
    AppendSink<RE::TESObjectLoadedEvent>(CreateEV({ "objectLoaded" }));
    AppendSink<RE::TESObjectREFRTranslationEvent>(
      CreateEV({ "translationFailed", "translationAlmostCompleted",
                 "translationCompleted" }));
    AppendSink<RE::TESOpenCloseEvent>(CreateEV({ "open", "close" }));
    AppendSink<RE::TESPackageEvent>(
      CreateEV({ "packageStart", "packageChange", "packageEnd" }));
    AppendSink<RE::TESPerkEntryRunEvent>(CreateEV({ "perkEntryRun" }));
    AppendSink<RE::TESPlayerBowShotEvent>(CreateEV({ "playerBowShot" }));
    AppendSink<RE::TESQuestInitEvent>(CreateEV({ "questInit" }));
    AppendSink<RE::TESQuestStageEvent>(CreateEV({ "questStage" }));
    AppendSink<RE::TESQuestStartStopEvent>(
      CreateEV({ "questStart", "questStop" }));
    AppendSink<RE::TESResetEvent>(CreateEV({ "reset" }));
    AppendSink<RE::TESSceneActionEvent>(CreateEV({ "sceneAction" }));
    AppendSink<RE::TESSellEvent>(CreateEV({ "sell" }));
    AppendSink<RE::TESSleepStartEvent>(CreateEV({ "sleepStart" }));
    AppendSink<RE::TESSleepStopEvent>(CreateEV({ "sleepStop" }));
    AppendSink<RE::TESSpellCastEvent>(CreateEV({ "spellCast" }));
    AppendSink<RE::TESSwitchRaceCompleteEvent>(
      CreateEV({ "switchRaceComplete" }));
    AppendSink<RE::TESTrackedStatsEvent>(CreateEV({ "trackedStats" }));
    AppendSink<RE::TESTriggerEnterEvent>(CreateEV({ "triggerEnter" }));
    AppendSink<RE::TESTriggerEvent>(CreateEV({ "trigger" }));
    AppendSink<RE::TESTriggerLeaveEvent>(CreateEV({ "triggerLeave" }));
    AppendSink<RE::TESUniqueIDChangeEvent>(CreateEV({ "uniqueIdChange" }));
    AppendSink<RE::TESWaitStartEvent>(CreateEV({ "waitStart" }));
    AppendSink<RE::TESWaitStopEvent>(CreateEV({ "waitStop" }));
  };
  EventHandlerScript(const EventHandlerScript&) = delete;
  EventHandlerScript(EventHandlerScript&&) = delete;

  ~EventHandlerScript() = default;
};
