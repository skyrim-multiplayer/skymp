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

EventResult EvEventHandlerMisc::ProcessEvent(
  const RE::MenuOpenCloseEvent* event,
  RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource)
{
  const char* menuName = event->menuName.c_str();

  if (event->opening) {
    EventsApi::SendMenuOpen(menuName);
  } else {
    EventsApi::SendMenuClose(menuName);
  }

  return EventResult::kContinue;
};

EventResult EvEventHandlerMisc::ProcessEvent(
  const RE::BGSFootstepEvent* event,
  RE::BSTEventSource<RE::BGSFootstepEvent>* eventSource)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("tag", JsValue::String(event->tag.c_str()));

    EventsApi::SendEvent("footstep", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

/* EventResult EvEventHandlerMisc::ProcessEvent(
  const RE::PositionPlayerEvent* event,
  RE::BSTEventSource<RE::PositionPlayerEvent>* eventSource)
{
  if (event == nullptr) {
    return EventResult::kContinue;
  }
  auto type = event->type;
  SkyrimPlatform::GetSingleton().AddUpdateTask([type] {
    auto obj = JsValue::Object();
    obj.SetProperty("eventType", JsValue::Double(static_cast<int>(type)));
    EventsApi::SendEvent("positionPlayer", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
} */
