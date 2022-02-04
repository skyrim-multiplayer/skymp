#pragma once

#include "EventHandlerBase.h"

class EventHandlerMisc final
  : public EventHandlerBase
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
    if (const auto ui = RE::UI::GetSingleton()) {
      AppendSink<RE::MenuOpenCloseEvent>(CreateEV({ "menuOpen", "menuClose" }),
                                         ui);
    }

    if (const auto manager = RE::BGSFootstepManager::GetSingleton()) {
      AppendSink(CreateEV({ "footstep" }), manager);
    }

    if (const auto pc = RE::PlayerCharacter::GetSingleton()) {
      AppendSink<RE::PositionPlayerEvent>(CreateEV({ "positionPlayer" }), pc);
    }
  }

  EventHandlerMisc(const EventHandlerMisc&) = delete;
  EventHandlerMisc(EventHandlerMisc&&) = delete;

  ~EventHandlerMisc() = default;
};
