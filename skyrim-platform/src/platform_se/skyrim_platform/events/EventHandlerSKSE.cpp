#include "EventHandlerSKSE.h"
#include "../EventsApi.h"
#include "EventUtils.h"
#include "../SkyrimPlatform.h"

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::ActionEvent* event,
  RE::BSTEventSource<SKSE::ActionEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto actorId = event->actor ? event->actor->formID : 0;
  auto sourceId = event->sourceForm ? event->sourceForm->formID : 0;

  if ((!event->actor || event->actor->formID != actorId) ||
      (!event->sourceForm || event->sourceForm->formID != sourceId)) {
    return EventResult::kContinue;
  }

  auto slot = to_underlying<SKSE::ActionEvent::Slot>(event->slot.get());

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "actor", event->actor, "Actor");
    AddProperty(&obj, "source", event->sourceForm, "Form");
    AddProperty(&obj, "slot", slot);

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

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::CameraEvent* event,
  RE::BSTEventSource<SKSE::CameraEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto oldStateId =
    to_underlying<RE::CameraStates::CameraState>(event->oldState->id);
  auto newStateId =
    to_underlying<RE::CameraStates::CameraState>(event->newState->id);

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "oldStateId", oldStateId);
    AddProperty(&obj, "newStateId", newStateId);

    EventsApi::SendEvent("cameraStateChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::CrosshairRefEvent* event,
  RE::BSTEventSource<SKSE::CrosshairRefEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto refr = event->crosshairRef ? event->crosshairRef.get() : nullptr;
  auto refrId = refr ? refr->formID : 0;

  if (!refr || refr->formID != refrId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "reference", refr, "ObjectReference");

    EventsApi::SendEvent("crosshairRefChanged", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::NiNodeUpdateEvent* event,
  RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  auto referenceId = event->reference ? event->reference->formID : 0;

  if (!event->reference || event->reference->formID != referenceId) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "reference", event->reference, "ObjectReference");

    EventsApi::SendEvent("niNodeUpdate", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}

EventResult EventHandlerSKSE::ProcessEvent(
  const SKSE::ModCallbackEvent* event,
  RE::BSTEventSource<SKSE::ModCallbackEvent>* eventSource)
{
  if (!event) {
    return EventResult::kContinue;
  }

  SkyrimPlatform::GetSingleton().AddUpdateTask([&] {
    auto obj = JsValue::Object();

    AddProperty(&obj, "sender", event->sender, "Form");
    AddProperty(&obj, "eventName", event->eventName.c_str());
    AddProperty(&obj, "strArg", event->strArg.c_str());
    AddProperty(&obj, "numArg", event->numArg);

    EventsApi::SendEvent("modEvent", { JsValue::Undefined(), obj });
  });

  return EventResult::kContinue;
}
