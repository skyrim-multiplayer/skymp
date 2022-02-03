#pragma once

#include "EventHandlerBase.h"

using EventResult = RE::BSEventNotifyControl;

class EventHandlerScript final
  : EventHandlerBase
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

  EventMap FetchEvents() override { return sinks; }

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
    AppendSink<RE::TESActivateEvent>(&std::vector{ "activate" });
    AppendSink<RE::TESActiveEffectApplyRemoveEvent>(
      &std::vector{ "effectStart", "effectFinish" });
    AppendSink<RE::TESActorLocationChangeEvent>(
      &std::vector{ "locationChanged" });
    AppendSink<RE::TESBookReadEvent>(&std::vector{ "bookRead" });
    AppendSink<RE::TESCellAttachDetachEvent>(
      &std::vector{ "cellAttach", "cellDetach" });
    AppendSink<RE::TESCellFullyLoadedEvent>(&std::vector{ "cellFullyLoaded" });
    AppendSink<RE::TESCombatEvent>(&std::vector{ "combatState" });
    AppendSink<RE::TESContainerChangedEvent>(
      &std::vector{ "containerChanged" });
    AppendSink<RE::TESDeathEvent>(&std::vector{ "deathEnd", "deathStart" });
    AppendSink<RE::TESDestructionStageChangedEvent>(
      &std::vector{ "destructionStageChanged" });
    AppendSink<RE::TESEnterBleedoutEvent>(&std::vector{ "enterBleedout" });
    AppendSink<RE::TESEquipEvent>(&std::vector{ "equip", "unequip" });
    AppendSink<RE::TESFastTravelEndEvent>(&std::vector{ "fastTravelEnd" });
    AppendSink<RE::TESFurnitureEvent>(
      &std::vector{ "furnitureExit", "furnitureEnter" });
    AppendSink<RE::TESGrabReleaseEvent>(&std::vector{ "grabRelease" });
    AppendSink<RE::TESHitEvent>(&std::vector{ "hit" });
    AppendSink<RE::TESInitScriptEvent>(&std::vector{ "scriptInit" });
    AppendSink<RE::TESLoadGameEvent>(&std::vector{ "loadGame" });
    AppendSink<RE::TESLockChangedEvent>(&std::vector{ "lockChanged" });
    AppendSink<RE::TESMagicEffectApplyEvent>(
      &std::vector{ "magicEffectApply" });
    AppendSink<RE::TESMagicWardHitEvent>(&std::vector{ "wardHit" });
    AppendSink<RE::TESMoveAttachDetachEvent>(
      &std::vector{ "moveAttachDetach" });
    AppendSink<RE::TESObjectLoadedEvent>(&std::vector{ "objectLoaded" });
    // AppendSink<RE::TESObjectREFRTranslationEvent>(&std::vector{
    // "translationFailed", "translationAlmostCompleted",
    // "translationCompleted" });
    AppendSink<RE::TESOpenCloseEvent>(&std::vector{ "open", "close" });
    AppendSink<RE::TESPackageEvent>(
      &std::vector{ "packageStart", "packageChange", "packageEnd" });
    AppendSink<RE::TESPerkEntryRunEvent>(&std::vector{ "perkEntryRun" });
    AppendSink<RE::TESPlayerBowShotEvent>(&std::vector{ "playerBowShot" });
    AppendSink<RE::TESQuestInitEvent>(&std::vector{ "questInit" });
    AppendSink<RE::TESQuestStageEvent>(&std::vector{ "questStage" });
    AppendSink<RE::TESQuestStartStopEvent>(
      &std::vector{ "questStart", "questStop" });
    AppendSink<RE::TESResetEvent>(&std::vector{ "reset" });
    // AppendSink<RE::TESSceneActionEvent>(&std::vector{ "sceneAction" });
    AppendSink<RE::TESSellEvent>(&std::vector{ "sell" });
    AppendSink<RE::TESSleepStartEvent>(&std::vector{ "sleepStart" });
    AppendSink<RE::TESSleepStopEvent>(&std::vector{ "sleepStop" });
    AppendSink<RE::TESSpellCastEvent>(&std::vector{ "spellCast" });
    AppendSink<RE::TESSwitchRaceCompleteEvent>(
      &std::vector{ "switchRaceComplete" });
    AppendSink<RE::TESTrackedStatsEvent>(&std::vector{ "trackedStats" });
    AppendSink<RE::TESTriggerEnterEvent>(&std::vector{ "triggerEnter" });
    AppendSink<RE::TESTriggerEvent>(&std::vector{ "trigger" });
    AppendSink<RE::TESTriggerLeaveEvent>(&std::vector{ "triggerLeave" });
    AppendSink<RE::TESUniqueIDChangeEvent>(&std::vector{ "uniqueIdChange" });
    AppendSink<RE::TESWaitStartEvent>(&std::vector{ "waitStart" });
    AppendSink<RE::TESWaitStopEvent>(&std::vector{ "waitStop" });
  };
  EventHandlerScript(const EventHandlerScript&) = delete;
  EventHandlerScript(EventHandlerScript&&) = delete;

  ~EventHandlerScript() = default;
};
