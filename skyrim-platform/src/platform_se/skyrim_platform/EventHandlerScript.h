#pragma once

using EventResult = RE::BSEventNotifyControl;

class EventHandlerScript final
  : public RE::BSTEventSink<RE::TESActivateEvent>
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

  static void RegisterSinks()
  {
    add_sink<RE::TESActivateEvent>();
    add_sink<RE::TESActiveEffectApplyRemoveEvent>();
    add_sink<RE::TESActorLocationChangeEvent>();
    add_sink<RE::TESBookReadEvent>();
    add_sink<RE::TESCellAttachDetachEvent>();
    add_sink<RE::TESCellFullyLoadedEvent>();
    add_sink<RE::TESCombatEvent>();
    add_sink<RE::TESContainerChangedEvent>();
    add_sink<RE::TESDeathEvent>();
    add_sink<RE::TESDestructionStageChangedEvent>();
    add_sink<RE::TESEnterBleedoutEvent>();
    add_sink<RE::TESEquipEvent>();
    add_sink<RE::TESFastTravelEndEvent>();
    add_sink<RE::TESFurnitureEvent>();
    add_sink<RE::TESGrabReleaseEvent>();
    add_sink<RE::TESHitEvent>();
    add_sink<RE::TESInitScriptEvent>();
    add_sink<RE::TESLoadGameEvent>();
    add_sink<RE::TESLockChangedEvent>();
    add_sink<RE::TESMagicEffectApplyEvent>();
    add_sink<RE::TESMagicWardHitEvent>();
    add_sink<RE::TESMoveAttachDetachEvent>();
    add_sink<RE::TESObjectLoadedEvent>();
    // add_sink<RE::TESObjectREFRTranslationEvent>();
    add_sink<RE::TESOpenCloseEvent>();
    add_sink<RE::TESPackageEvent>();
    add_sink<RE::TESPerkEntryRunEvent>();
    add_sink<RE::TESPlayerBowShotEvent>();
    add_sink<RE::TESQuestInitEvent>();
    add_sink<RE::TESQuestStageEvent>();
    add_sink<RE::TESQuestStartStopEvent>();
    add_sink<RE::TESResetEvent>();
    // add_sink<RE::TESSceneActionEvent>();
    add_sink<RE::TESSellEvent>();
    add_sink<RE::TESSleepStartEvent>();
    add_sink<RE::TESSleepStopEvent>();
    add_sink<RE::TESSpellCastEvent>();
    add_sink<RE::TESSwitchRaceCompleteEvent>();
    add_sink<RE::TESTrackedStatsEvent>();
    add_sink<RE::TESTriggerEnterEvent>();
    add_sink<RE::TESTriggerEvent>();
    add_sink<RE::TESTriggerLeaveEvent>();
    add_sink<RE::TESUniqueIDChangeEvent>();
    add_sink<RE::TESWaitStartEvent>();
    add_sink<RE::TESWaitStopEvent>();
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
  EventHandlerScript() = default;
  EventHandlerScript(const EventHandlerScript&) = delete;
  EventHandlerScript(EventHandlerScript&&) = delete;

  ~EventHandlerScript() = default;

  template <class T>
  static void add_sink()
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<T>(GetSingleton());
      logger::info("Registered {} handler"sv, typeid(T).name());
    }
  }
};
