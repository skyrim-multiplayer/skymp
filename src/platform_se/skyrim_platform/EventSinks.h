#pragma once
#include "NullPointerException.h"

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

class EventSinks
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
  , public RE::BSTEventSink<RE::TESGrabReleaseEvent>
  , public RE::BSTEventSink<RE::TESLockChangedEvent>
  , public RE::BSTEventSink<RE::TESMoveAttachDetachEvent>
  , public RE::BSTEventSink<RE::TESObjectLoadedEvent>
  , public RE::BSTEventSink<RE::TESWaitStopEvent>
  , public RE::BSTEventSink<RE::TESActivateEvent>

{
public:
  EventSinks()
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
      dynamic_cast<RE::BSTEventSink<RE::TESLockChangedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESObjectLoadedEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESWaitStopEvent>*>(this));

    holder->AddEventSink(
      dynamic_cast<RE::BSTEventSink<RE::TESActiveEffectApplyRemoveEvent>*>(
        this));
  }

private:
  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESActivateEvent* event_,
    RE::BSTEventSource<RE::TESActivateEvent>* eventSource) override;

  RE::BSEventNotifyControl ProcessEvent(
    const RE::TESMoveAttachDetachEvent* event_,
    RE::BSTEventSource<RE::TESMoveAttachDetachEvent>* eventSource) override;

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
};