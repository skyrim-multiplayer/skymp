#include "EventHandlerSKSE.h"
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

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::ActionEvent* event,
  RE::BSTEventSource<SKSE::ActionEvent>* eventSource)
{
  auto actorId = event->actor ? event->actor->formID : 0;
  auto sourceId = event->sourceForm ? event->sourceForm->formID : 0;

  if ((!event->actor || event->actor->formID != actorId) ||
      (!event->sourceForm || event->sourceForm->formID != sourceId)) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("actor", CreateObject("Actor", event->actor));
    obj.SetProperty("source", CreateObject("Form", event->sourceForm));
    obj.SetProperty("slot",
                    JsValue::Double(static_cast<double>(event->slot.get())));

    switch (event->type.get()) {
      case SKSE::ActionEvent::Type::kWeaponSwing: {
        EventsApi::SendEvent("actionWeaponSwing",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBeginDraw: {
        EventsApi::SendEvent("actionBeginDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kEndDraw: {
        EventsApi::SendEvent("actionEndDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBowDraw: {
        EventsApi::SendEvent("actionBowDraw", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBowRelease: {
        EventsApi::SendEvent("actionBowRelease",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kBeginSheathe: {
        EventsApi::SendEvent("actionBeginSheathe",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kEndSheathe: {
        EventsApi::SendEvent("actionEndSheathe",
                             { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kSpellCast: {
        EventsApi::SendEvent("actionSpellCast", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kSpellFire: {
        EventsApi::SendEvent("actionSpellFire", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kVoiceCast: {
        EventsApi::SendEvent("actionVoiceCast", { JsValue::Undefined(), obj });
        break;
      }
      case SKSE::ActionEvent::Type::kVoiceFire: {
        EventsApi::SendEvent("actionVoiceFire", { JsValue::Undefined(), obj });
        break;
      }
    }
  });

  return EventResult::kContinue;
}

// TODO: Fix this
EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::CameraEvent* event,
  RE::BSTEventSource<SKSE::CameraEvent>* eventSource)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();
    obj.SetProperty("oldStateId", JsValue::Double(event->oldStateId));
    obj.SetProperty("newStateId", JsValue::Double(event->newStateId));
    EventsApi::SendEvent("cameraStateChanged", { JsValue::Undefined(), obj });
  });
  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::CrosshairRefEvent* event,
  RE::BSTEventSource<SKSE::CrosshairRefEvent>* eventSource)
{
  auto refr = event->crosshairRef ? event->crosshairRef.get() : nullptr;
  auto refrId = refr ? refr->formID : 0;

  if (!refr || refr->formID != refrId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("reference", CreateObject("ObjectReference", refr));

    EventsApi::SendEvent("crosshairRefChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::NiNodeUpdateEvent* event,
  RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* eventSource)
{
  auto referenceId = event->reference ? event->reference->formID : 0;

  if (!event->reference || event->reference->formID != referenceId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("reference",
                    CreateObject("ObjectReference", event->reference));

    EventsApi::SendEvent("niNodeUpdate", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::ModCallbackEvent* event,
  RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource)
{
  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    obj.SetProperty("sender", CreateObject("Form", event->sender));
    obj.SetProperty("eventName", JsValue::String(event->eventName.c_str()));
    obj.SetProperty("strArg", JsValue::String(event->strArg.c_str()));
    obj.SetProperty("numArg", JsValue::Double(event->numArg));

    EventsApi::SendEvent("modEvent", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}