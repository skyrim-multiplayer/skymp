#pragma once

using EventResult = RE::BSEventNotifyControl;

class EventHandlerMisc final
  : public RE::BSTEventSink<RE::BGSFootstepEvent>
  , public RE::BSTEventSink<RE::MenuOpenCloseEvent>
  , public RE::BSTEventSink<RE::PositionPlayerEvent>
{
public:
  [[nodiscard]] static EventHandlerMisc* GetSingleton()
  {
    static EventHandlerMisc singleton;
    return &singleton;
  }

  static void RegisterSinks()
  {
    if (const auto manager = RE::BGSFootstepManager::GetSingleton()) {
      manager->AddEventSink(GetSingleton());
    }

    if (const auto ui = RE::UI::GetSingleton()) {
      ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(
        GetSingleton());
    }

    if (const auto pc =
          RE::PlayerCharacter::GetSingleton()
            ->As<RE::BSTEventSource<RE::PositionPlayerEvent>>()) {
      pc->AddEventSink(GetSingleton());
    }
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
  EventHandlerMisc() = default;
  EventHandlerMisc(const EventHandlerMisc&) = delete;
  EventHandlerMisc(EventHandlerMisc&&) = delete;

  ~EventHandlerMisc() = default;
};
