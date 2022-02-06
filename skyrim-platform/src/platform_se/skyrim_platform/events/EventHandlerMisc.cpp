#include "EventHandlerMisc.h"
#include "../EventsApi.h"
#include "../SkyrimPlatform.h"
#include "EventUtils.h"

EventResult EventHandlerMisc::ProcessEvent(
  const RE::MenuOpenCloseEvent* event,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
{
  if (!event) {
    return EventResult::kContinue;
  }

  const char* menuName = event->menuName.c_str();

  SkyrimPlatform::GetSingleton().AddUpdateTask([=] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "name", menuName);

    if (event->opening) {
      EventsApi::SendEvent("menuOpen", { JsValue::Undefined(), obj });
    } else {
      EventsApi::SendEvent("menuClose", { JsValue::Undefined(), obj });
    }
  });

  return EventResult::kContinue;
};

EventResult EventHandlerMisc::ProcessEvent(
  const RE::BGSFootstepEvent* event, RE::BSTEventSource<RE::BGSFootstepEvent>*)
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
  RE::BSTEventSource<RE::PositionPlayerEvent>*)
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
