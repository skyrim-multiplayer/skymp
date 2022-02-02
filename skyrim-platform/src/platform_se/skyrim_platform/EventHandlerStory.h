#pragma once

using EventResult = RE::BSEventNotifyControl;

class EventHandlerStory final
  : public RE::BSTEventSink<RE::ActorKill::Event>
  , public RE::BSTEventSink<RE::BooksRead::Event>
  , public RE::BSTEventSink<RE::CriticalHit::Event>
  , public RE::BSTEventSink<RE::DisarmedEvent::Event>
  , public RE::BSTEventSink<RE::DragonSoulsGained::Event>
  , public RE::BSTEventSink<RE::ItemHarvested::Event>
  , public RE::BSTEventSink<RE::LevelIncrease::Event>
  , public RE::BSTEventSink<RE::LocationDiscovery::Event>
  , public RE::BSTEventSink<RE::ShoutAttack::Event>
  , public RE::BSTEventSink<RE::SkillIncrease::Event>
  , public RE::BSTEventSink<RE::SoulsTrapped::Event>
  , public RE::BSTEventSink<RE::SpellsLearned::Event>
{
public:
  [[nodiscard]] static EventHandlerStory* GetSingleton()
  {
    static EventHandlerStory singleton;
    return &singleton;
  }

  static void RegisterSinks()
  {
    add_sink<RE::ActorKill>();
    add_sink<RE::BooksRead>();
    add_sink<RE::CriticalHit>();
    add_sink<RE::DisarmedEvent>();
    add_sink<RE::DragonSoulsGained>();
    add_sink<RE::ItemHarvested>();
    add_sink<RE::LevelIncrease>();
    add_sink<RE::LocationDiscovery>();
    add_sink<RE::ShoutAttack>();
    add_sink<RE::SkillIncrease>();
    add_sink<RE::SoulsTrapped>();
    add_sink<RE::SpellsLearned>();
  }

  EventResult ProcessEvent(const RE::ActorKill::Event* a_event,
                           RE::BSTEventSource<RE::ActorKill::Event>*) override;

  EventResult ProcessEvent(const RE::BooksRead::Event* a_event,
                           RE::BSTEventSource<RE::BooksRead::Event>*) override;

  EventResult ProcessEvent(
    const RE::CriticalHit::Event* a_event,
    RE::BSTEventSource<RE::CriticalHit::Event>*) override;

  EventResult ProcessEvent(
    const RE::DisarmedEvent::Event* a_event,
    RE::BSTEventSource<RE::DisarmedEvent::Event>*) override;

  EventResult ProcessEvent(
    const RE::DragonSoulsGained::Event* a_event,
    RE::BSTEventSource<RE::DragonSoulsGained::Event>*) override;

  EventResult ProcessEvent(
    const RE::ItemHarvested::Event* a_event,
    RE::BSTEventSource<RE::ItemHarvested::Event>*) override;

  EventResult ProcessEvent(
    const RE::LevelIncrease::Event* a_event,
    RE::BSTEventSource<RE::LevelIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::LocationDiscovery::Event* a_event,
    RE::BSTEventSource<RE::LocationDiscovery::Event>*) override;

  EventResult ProcessEvent(
    const RE::ShoutAttack::Event* a_event,
    RE::BSTEventSource<RE::ShoutAttack::Event>*) override;

  EventResult ProcessEvent(
    const RE::SkillIncrease::Event* a_event,
    RE::BSTEventSource<RE::SkillIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::SoulsTrapped::Event* a_event,
    RE::BSTEventSource<RE::SoulsTrapped::Event>*) override;

  EventResult ProcessEvent(
    const RE::SpellsLearned::Event* a_event,
    RE::BSTEventSource<RE::SpellsLearned::Event>*) override;

private:
  EventHandlerStory() = default;
  EventHandlerStory(const EventHandlerStory&) = delete;
  EventHandlerStory(EventHandlerStory&&) = delete;

  ~EventHandlerStory() = default;

  template <class T>
  static void add_sink()
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink<T::Event>(GetSingleton());
      logger::info("Registered {} handler"sv, typeid(T::Event).name());
    }
  }
};
