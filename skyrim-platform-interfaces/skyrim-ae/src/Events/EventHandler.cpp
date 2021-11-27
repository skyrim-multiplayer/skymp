#include "EventHandler.h"

namespace Events {

/**
 * At process event we get arguments which we need to pass
 * to Platform, where they get to be evaluated and cast into JS obj
 * after that they are passed to Event API which sends them to subscribers
 *
 * Since we have different arguments for each event and Platform
 * doesn't know neither about types nor about argument quantity
 * we probably gotta have separate functions when passing event to Platform
 * so it can resolve arguments into objects,
 * so GameInterface should have a separate function for each event?
 */

EventResult EventHandler::ProcessEvent(
  const RE::MenuOpenCloseEvent* a_event,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActivateEvent* a_event,
  RE::BSTEventSource<RE::TESActivateEvent>*)
{
  /* => Interface API | No need to check here?
  if are gonna be doing native calls there will be checks anyway */

  /* auto targetRefr = a_event ? a_event->objectActivated.get()
                            : nullptr; // objectActivated = target
  auto casterRefr =
    a_event ? a_event->actionRef.get() : nullptr; // actionRef = caster

  auto targetId = targetRefr ? targetRefr->formID : 0;
  auto casterId = casterRefr ? casterRefr->formID : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask(
    [targetId, casterId, targetRefr, casterRefr] {
      auto obj = JsValue::Object();

      auto target = RE::TESForm::LookupByID(targetId);
      target = target == targetRefr ? target : nullptr;
      obj.SetProperty("target", CreateObject("ObjectReference", target));

      auto caster = RE::TESForm::LookupByID(casterId);
      caster = caster == casterRefr ? caster : nullptr;
      obj.SetProperty("caster", CreateObject("ObjectReference", caster));

      auto targetRefr_ = reinterpret_cast<RE::TESObjectREFR*>(target);

      obj.SetProperty("isCrimeToActivate",
                      targetRefr_
                        ? JsValue::Bool(targetRefr_->IsCrimeToActivate())
                        : JsValue::Undefined());

      EventsApi::SendEvent("activate", { JsValue::Undefined(), obj });
    }); */

  return EventResult::kContinue;
}

EventResult EventHandler::ProcessEvent(
  const RE::TESActiveEffectApplyRemoveEvent* a_event,
  RE::BSTEventSource<RE::TESActiveEffectApplyRemoveEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESCellFullyLoadedEvent* a_event,
  RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*)
{
}

EventResult EventHandler::ProcessEvent(const RE::TESCombatEvent* a_event,
                                       RE::BSTEventSource<RE::TESCombatEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESContainerChangedEvent* a_event,
  RE::BSTEventSource<RE::TESContainerChangedEvent>*)
{
}

EventResult EventHandler::ProcessEvent(const RE::TESDeathEvent* a_event,
                                       RE::BSTEventSource<RE::TESDeathEvent>*)
{
}

EventResult EventHandler::ProcessEvent(const RE::TESEquipEvent* a_event,
                                       RE::BSTEventSource<RE::TESEquipEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESGrabReleaseEvent* a_event,
  RE::BSTEventSource<RE::TESGrabReleaseEvent>*)
{
}

EventResult EventHandler::ProcessEvent(const RE::TESHitEvent* a_event,
                                       RE::BSTEventSource<RE::TESHitEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESInitScriptEvent* a_event,
  RE::BSTEventSource<RE::TESInitScriptEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESLoadGameEvent* a_event,
  RE::BSTEventSource<RE::TESLoadGameEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESLockChangedEvent* a_event,
  RE::BSTEventSource<RE::TESLockChangedEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMagicEffectApplyEvent* a_event,
  RE::BSTEventSource<RE::TESMagicEffectApplyEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESMoveAttachDetachEvent* a_event,
  RE::BSTEventSource<RE::TESMoveAttachDetachEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESObjectLoadedEvent* a_event,
  RE::BSTEventSource<RE::TESObjectLoadedEvent>*)
{
}

EventResult EventHandler::ProcessEvent(const RE::TESResetEvent* a_event,
                                       RE::BSTEventSource<RE::TESResetEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESSwitchRaceCompleteEvent* a_event,
  RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESTrackedStatsEvent* a_event,
  RE::BSTEventSource<RE::TESTrackedStatsEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESUniqueIDChangeEvent* a_event,
  RE::BSTEventSource<RE::TESUniqueIDChangeEvent>*)
{
}

EventResult EventHandler::ProcessEvent(
  const RE::TESWaitStopEvent* a_event,
  RE::BSTEventSource<RE::TESWaitStopEvent>*)
{
  /* auto interrupted = event ? event->interrupted : 0;

  SkyrimPlatform::GetSingleton().AddUpdateTask([interrupted] {
    auto obj = JsValue::Object();

    obj.SetProperty("isInterrupted", JsValue::Bool(interrupted));

    EventsApi::SendEvent("waitStop", { JsValue::Undefined(), obj });
  }); */
}

}
