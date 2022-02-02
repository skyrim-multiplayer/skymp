#include "EventHandlerMisc.h"
#include "EventUtils.h"
#include "EventsApi.h"
#include "SkyrimPlatform.h"

EventResult EventHandlerMisc::ProcessEvent(
  const RE::MenuOpenCloseEvent* event,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  const char* menuName = event->menuName.c_str();

  if (event->opening) {
    EventsApi::SendMenuOpen(menuName);
  } else {
    EventsApi::SendMenuClose(menuName);
  }

  return EventResult::kContinue;
};

EventResult EventHandlerMisc::ProcessEvent(
  const RE::BGSFootstepEvent* event,
  RE::BSTEventSource<RE::BGSFootstepEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "tag", event->tag.c_str());

    EventsApi::SendEvent("footstep", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerMisc::ProcessEvent(
  const RE::PositionPlayerEvent* event,
  RE::BSTEventSource<RE::PositionPlayerEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto type =
    to_underlying<RE::PositionPlayerEvent::EVENT_TYPE>(event->type.get());

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "eventType", type);

    EventsApi::SendEvent("positionPlayer", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}
