#pragma once

using EventResult = RE::BSEventNotifyControl;

class EventHandlerSKSE final
  : public RE::BSTEventSink<SKSE::ActionEvent>
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

  static void RegisterSinks()
  {
    SKSE::GetActionEventSource()->AddEventSink(GetSingleton());
    SKSE::GetCameraEventSource()->AddEventSink(GetSingleton());
    SKSE::GetCrosshairRefEventSource()->AddEventSink(GetSingleton());
    SKSE::GetModCallbackEventSource()->AddEventSink(GetSingleton());
    SKSE::GetNiNodeUpdateEventSource()->AddEventSink(GetSingleton());
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
  EventHandlerSKSE() = default;
  EventHandlerSKSE(const EventHandlerSKSE&) = delete;
  EventHandlerSKSE(EventHandlerSKSE&&) = delete;

  ~EventHandlerSKSE() = default;
};
