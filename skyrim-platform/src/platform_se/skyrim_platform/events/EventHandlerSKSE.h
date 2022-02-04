#pragma once

#include "EventHandlerBase.h"

class EventHandlerSKSE final
  : public EventHandlerBase
  , public RE::BSTEventSink<SKSE::ActionEvent>
  , public RE::BSTEventSink<SKSE::CameraEvent>
  , public RE::BSTEventSink<SKSE::CrosshairRefEvent>
  , public RE::BSTEventSink<SKSE::NiNodeUpdateEvent>
  , public RE::BSTEventSink<SKSE::ModCallbackEvent>
{
public:
  [[nodiscard]] static EventHandlerSKSE* GetSingleton()
  {
    static EventHandlerSKSE singleton;
    return &singleton;
  }

  EventResult ProcessEvent(const SKSE::ActionEvent* event,
                           RE::BSTEventSource<SKSE::ActionEvent>*) override;

  EventResult ProcessEvent(const SKSE::CameraEvent* event,
                           RE::BSTEventSource<SKSE::CameraEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::CrosshairRefEvent* event,
    RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::NiNodeUpdateEvent* event,
    RE::BSTEventSource<SKSE::NiNodeUpdateEvent>*) override;

  EventResult ProcessEvent(
    const SKSE::ModCallbackEvent* event,
    RE::BSTEventSource<SKSE::ModCallbackEvent>*) override;

private:
  EventHandlerSKSE()
  {
    if (const auto holder = SKSE::GetActionEventSource())
      AppendSink(
        CreateEV({ "actionWeaponSwing", "actionBeginDraw", "actionEndDraw",
                   "actionBowDraw", "actionBowRelease", "actionBeginSheathe",
                   "actionEndSheathe", "actionSpellCast", "actionSpellFire",
                   "actionVoiceCast", "actionVoiceFire" }),
        holder);

    if (const auto holder = SKSE::GetCameraEventSource())
      AppendSink(CreateEV({ "cameraStateChanged" }), holder);

    if (const auto holder = SKSE::GetCrosshairRefEventSource())
      AppendSink(CreateEV({ "crosshairRefChanged" }), holder);

    if (const auto holder = SKSE::GetModCallbackEventSource())
      AppendSink(CreateEV({ "modEvent" }), holder);

    if (const auto holder = SKSE::GetNiNodeUpdateEventSource())
      AppendSink(CreateEV({ "niNodeUpdate" }), holder);
  };
  EventHandlerSKSE(const EventHandlerSKSE&) = delete;
  EventHandlerSKSE(EventHandlerSKSE&&) = delete;

  ~EventHandlerSKSE() = default;
};
