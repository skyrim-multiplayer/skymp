#pragma once

#include "EventHandlerBase.h"

using EventResult = RE::BSEventNotifyControl;

class EventHandlerMisc final
  : EventHandlerBase
  , public RE::BSTEventSink<RE::BGSFootstepEvent>
  , public RE::BSTEventSink<RE::MenuOpenCloseEvent>
  , public RE::BSTEventSink<RE::PositionPlayerEvent>
{
public:
  [[nodiscard]] static EventHandlerMisc* GetSingleton()
  {
    static EventHandlerMisc singleton;
    return &singleton;
  }

  EventResult ProcessEvent(const RE::BGSFootstepEvent* event,
                           RE::BSTEventSource<RE::BGSFootstepEvent>*) override;

  EventResult ProcessEvent(
    const RE::MenuOpenCloseEvent* event,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

  EventResult ProcessEvent(
    const RE::PositionPlayerEvent* event,
    RE::BSTEventSource<RE::PositionPlayerEvent>*) override;

private:
  EventHandlerMisc()
  {
    // No implementation for this event on JS side atm,
    // so just register it to proc constantly
    if (const auto ui = RE::UI::GetSingleton()) {
      ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(
        GetSingleton());
    }

    if (const auto manager = RE::BGSFootstepManager::GetSingleton()) {
      AppendSink(&std::vector{ "footstep" }, manager);
    }

    if (const auto pc =
          RE::PlayerCharacter::GetSingleton()
            ->As<RE::BSTEventSource<RE::PositionPlayerEvent>>()) {
      AppendSink(&std::vector{ "positionPlayer" }, pc);
    }
  }
  EventHandlerMisc(const EventHandlerMisc&) = delete;
  EventHandlerMisc(EventHandlerMisc&&) = delete;

  ~EventHandlerMisc() = default;
};
