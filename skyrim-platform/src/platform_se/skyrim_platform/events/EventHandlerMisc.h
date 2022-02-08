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
    AppendSink<RE::UI, RE::MenuOpenCloseEvent>(
      CreateEventList({ "menuOpen", "menuClose" }));

    AppendSink<RE::BGSFootstepManager, RE::BGSFootstepEvent>(
      CreateEventList({ "footstep" }));

    AppendSink<RE::PlayerCharacter, RE::PositionPlayerEvent>(
      CreateEventList({ "positionPlayer" }));
  }

  EventHandlerMisc(const EventHandlerMisc&) = delete;
  EventHandlerMisc(EventHandlerMisc&&) = delete;

  ~EventHandlerMisc() = default;
};
