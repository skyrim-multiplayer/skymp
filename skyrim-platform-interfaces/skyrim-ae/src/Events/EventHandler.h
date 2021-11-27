#pragma once

using EventResult = RE::BSEventNotifyControl;

namespace Events {

class EventHandler final

  : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
  , public RE::BSTEventSink<RE::TESActivateEvent>
  , public RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>
  , public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>
  , public RE::BSTEventSink<RE::TESCombatEvent>
  , public RE::BSTEventSink<RE::TESContainerChangedEvent>
  , public RE::BSTEventSink<RE::TESDeathEvent>
  , public RE::BSTEventSink<RE::TESEquipEvent>
  , public RE::BSTEventSink<RE::TESGrabReleaseEvent>
  , public RE::BSTEventSink<RE::TESHitEvent>
  , public RE::BSTEventSink<RE::TESInitScriptEvent>
  , public RE::BSTEventSink<RE::TESLoadGameEvent>
  , public RE::BSTEventSink<RE::TESLockChangedEvent>
  , public RE::BSTEventSink<RE::TESMagicEffectApplyEvent>
  , public RE::BSTEventSink<RE::TESMoveAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESObjectLoadedEvent>
  , public RE::BSTEventSink<RE::TESResetEvent>
  , public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent>
  , public RE::BSTEventSink<RE::TESTrackedStatsEvent>
  , public RE::BSTEventSink<RE::TESUniqueIDChangeEvent>
  , public RE::BSTEventSink<RE::TESWaitStopEvent>
{
public:
  [[nodiscard]] static EventHandler* GetSingleton()
  {
    static EventHandler singleton;
    return &singleton;
  }

  /* consider SKSE/Events.h */
  static void Register()
  {
    auto ui = RE::UI::GetSingleton();
    if (!ui) {
      logger::critical("ui exception @ event handler");
    }

    ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(GetSingleton());

    auto holder = RE::ScriptEventSourceHolder::GetSingleton();
    if (holder) {
      holder->AddEventSink<RE::TESActivateEvent>(GetSingleton());
      holder->AddEventSink<RE::TESActiveEffectApplyRemoveEvent>(
        GetSingleton());
      holder->AddEventSink<RE::TESCellFullyLoadedEvent>(GetSingleton());
      holder->AddEventSink<RE::TESCombatEvent>(GetSingleton());
      holder->AddEventSink<RE::TESContainerChangedEvent>(GetSingleton());
      holder->AddEventSink<RE::TESDeathEvent>(GetSingleton());
      holder->AddEventSink<RE::TESEquipEvent>(GetSingleton());
      holder->AddEventSink<RE::TESGrabReleaseEvent>(GetSingleton());
      holder->AddEventSink<RE::TESHitEvent>(GetSingleton());
      holder->AddEventSink<RE::TESInitScriptEvent>(GetSingleton());
      holder->AddEventSink<RE::TESLoadGameEvent>(GetSingleton());
      holder->AddEventSink<RE::TESLockChangedEvent>(GetSingleton());
      holder->AddEventSink<RE::TESMagicEffectApplyEvent>(GetSingleton());
      holder->AddEventSink<RE::TESMoveAttachDetachEvent>(GetSingleton());
      holder->AddEventSink<RE::TESObjectLoadedEvent>(GetSingleton());
      holder->AddEventSink<RE::TESResetEvent>(GetSingleton());
      holder->AddEventSink<RE::TESSwitchRaceCompleteEvent>(GetSingleton());
      holder->AddEventSink<RE::TESTrackedStatsEvent>(GetSingleton());
      holder->AddEventSink<RE::TESUniqueIDChangeEvent>(GetSingleton());
      holder->AddEventSink<RE::TESWaitStopEvent>(GetSingleton());
    }
  }

  EventResult ProcessEvent(
    const RE::MenuOpenCloseEvent* a_event,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

  EventResult ProcessEvent(const RE::TESActivateEvent* a_event,
                           RE::BSTEventSource<RE::TESActivateEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESActiveEffectApplyRemoveEvent* a_event,
    RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESCellFullyLoadedEvent* a_event,
    RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;

  EventResult ProcessEvent(const RE::TESCombatEvent* a_event,
                           RE::BSTEventSource<RE::TESCombatEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESContainerChangedEvent* a_event,
    RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;

  EventResult ProcessEvent(const RE::TESDeathEvent* a_event,
                           RE::BSTEventSource<RE::TESDeathEvent>*) override;

  EventResult ProcessEvent(const RE::TESEquipEvent* a_event,
                           RE::BSTEventSource<RE::TESEquipEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESGrabReleaseEvent* a_event,
    RE::BSTEventSource<RE::TESGrabReleaseEvent>*) override;

  EventResult ProcessEvent(const RE::TESHitEvent* a_event,
                           RE::BSTEventSource<RE::TESHitEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESInitScriptEvent* a_event,
    RE::BSTEventSource<RE::TESInitScriptEvent>*) override;

  EventResult ProcessEvent(const RE::TESLoadGameEvent* a_event,
                           RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESLockChangedEvent* a_event,
    RE::BSTEventSource<RE::TESLockChangedEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMagicEffectApplyEvent* a_event,
    RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESMoveAttachDetachEvent* a_event,
    RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESObjectLoadedEvent* a_event,
    RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;

  EventResult ProcessEvent(const RE::TESResetEvent* a_event,
                           RE::BSTEventSource<RE::TESResetEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESSwitchRaceCompleteEvent* a_event,
    RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESTrackedStatsEvent* a_event,
    RE::BSTEventSource<RE::TESTrackedStatsEvent>*) override;

  EventResult ProcessEvent(
    const RE::TESUniqueIDChangeEvent* a_event,
    RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*) override;

  EventResult ProcessEvent(const RE::TESWaitStopEvent* a_event,
                           RE::BSTEventSource<RE::TESWaitStopEvent>*) override;

private:
  EventHandler() = default;
  EventHandler(const EventHandler&) = delete;
  EventHandler(EventHandler&&) = delete;

  ~EventHandler() = default;

  EventHandler& operator=(const EventHandler&) = delete;
  EventHandler& operator=(EventHandler&&) = delete;
};
}
