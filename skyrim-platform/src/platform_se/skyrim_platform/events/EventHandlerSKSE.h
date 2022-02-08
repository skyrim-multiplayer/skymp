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
  template <class E>
  void AppendSink(std::vector<const char*>* events,
                  RE::BSTEventSource<E>* holder)
  {
    auto handler = EventHandlerSKSE::GetSingleton();
    if (handler->sinks->contains(events)) {
      logger::critical(
        "Attempt to append EventSink for {} failed. Already exists.",
        typeid(E).name());

      return;
    }

    Sink* sink = new Sink(
      [sink, holder] {
        auto handler = EventHandlerSKSE::GetSingleton();
        handler->add_sink(holder);
        handler->activeSinksEmplace(sink);
      },
      [sink, holder] {
        auto handler = EventHandlerSKSE::GetSingleton();
        handler->remove_sink(holder);
        handler->activeSinksErase(sink);
      },
      [sink](bool) {
        auto handler = EventHandlerSKSE::GetSingleton();
        return handler->IsActiveSink(sink);
      },
      events);

    handler->sinks->emplace(events, sink);
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
      AppendSink(CreateEventList(
                   { "actionWeaponSwing", "actionBeginDraw", "actionEndDraw",
                     "actionBowDraw", "actionBowRelease", "actionBeginSheathe",
                     "actionEndSheathe", "actionSpellCast", "actionSpellFire",
                     "actionVoiceCast", "actionVoiceFire" }),
                 holder);

    if (const auto holder = SKSE::GetCameraEventSource())
      AppendSink(CreateEventList({ "cameraStateChanged" }), holder);

    if (const auto holder = SKSE::GetCrosshairRefEventSource())
      AppendSink(CreateEventList({ "crosshairRefChanged" }), holder);

    if (const auto holder = SKSE::GetModCallbackEventSource())
      AppendSink(CreateEventList({ "modEvent" }), holder);

    if (const auto holder = SKSE::GetNiNodeUpdateEventSource())
      AppendSink(CreateEventList({ "niNodeUpdate" }), holder);
  };
  EventHandlerSKSE(const EventHandlerSKSE&) = delete;
  EventHandlerSKSE(EventHandlerSKSE&&) = delete;

  ~EventHandlerSKSE() = default;
};
