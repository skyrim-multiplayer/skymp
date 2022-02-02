#include "EventHandlerMisc.h"
#include "EventsApi.h"
#include "NativeValueCasts.h"
#include "SkyrimPlatform.h"

namespace {
JsValue CreateObject(const char* type, void* form)
{
  return form ? NativeValueCasts::NativeObjectToJsObject(
                  std::make_shared<CallNative::Object>(type, form))
              : JsValue::Null();
}
}

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

    obj.SetProperty("tag", JsValue::String(event->tag.c_str()));

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
    obj.SetProperty("eventType", JsValue::Double(type));
    EventsApi::SendEvent("positionPlayer", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}
