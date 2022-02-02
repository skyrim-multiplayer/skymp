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
    auto footstepHolder = RE::BGSFootstepManager::GetSingleton();
    auto footstepEventSource = footstepHolder
      ? static_cast<RE::BSTEventSource<RE::BGSFootstepEvent>*>(footstepHolder)
      : throw NullPointerException("footstepHolder");
    footstepEventSource->AddEventSink(GetSingleton());

    auto ui = RE::UI::GetSingleton();
    if (!ui) {
      throw NullPointerException("ui");
    }
    ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(GetSingleton());

    auto playerCharacterHolder = RE::PlayerCharacter::GetSingleton();
    auto playerCharacterEventSource = playerCharacterHolder
      ? static_cast<RE::BSTEventSource<RE::PositionPlayerEvent>*>(
          playerCharacterHolder)
      : throw NullPointerException("playerCharacterHolder");
    playerCharacterEventSource->AddEventSink(GetSingleton());
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
