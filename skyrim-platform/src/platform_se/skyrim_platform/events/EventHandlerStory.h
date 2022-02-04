#pragma once

#include "EventHandlerBase.h"

using EventResult = RE::BSEventNotifyControl;

class EventHandlerStory final
  : EventHandlerBase
  , public RE::BSTEventSink<RE::ActorKill::Event>
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

  EventMap FetchEvents() override { return sinks; }

  EventResult ProcessEvent(const RE::ActorKill::Event* event,
                           RE::BSTEventSource<RE::ActorKill::Event>*) override;

  EventResult ProcessEvent(const RE::BooksRead::Event* event,
                           RE::BSTEventSource<RE::BooksRead::Event>*) override;

  EventResult ProcessEvent(
    const RE::CriticalHit::Event* event,
    RE::BSTEventSource<RE::CriticalHit::Event>*) override;

  EventResult ProcessEvent(
    const RE::DisarmedEvent::Event* event,
    RE::BSTEventSource<RE::DisarmedEvent::Event>*) override;

  EventResult ProcessEvent(
    const RE::DragonSoulsGained::Event* event,
    RE::BSTEventSource<RE::DragonSoulsGained::Event>*) override;

  EventResult ProcessEvent(
    const RE::ItemHarvested::Event* event,
    RE::BSTEventSource<RE::ItemHarvested::Event>*) override;

  EventResult ProcessEvent(
    const RE::LevelIncrease::Event* event,
    RE::BSTEventSource<RE::LevelIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::LocationDiscovery::Event* event,
    RE::BSTEventSource<RE::LocationDiscovery::Event>*) override;

  EventResult ProcessEvent(
    const RE::ShoutAttack::Event* event,
    RE::BSTEventSource<RE::ShoutAttack::Event>*) override;

  EventResult ProcessEvent(
    const RE::SkillIncrease::Event* event,
    RE::BSTEventSource<RE::SkillIncrease::Event>*) override;

  EventResult ProcessEvent(
    const RE::SoulsTrapped::Event* event,
    RE::BSTEventSource<RE::SoulsTrapped::Event>*) override;

  EventResult ProcessEvent(
    const RE::SpellsLearned::Event* event,
    RE::BSTEventSource<RE::SpellsLearned::Event>*) override;

private:
  EventHandlerStory()
  {
    // TODO: implement these on JS side
    AppendSink<RE::ActorKill, RE::ActorKill::Event>(
      &std::vector{ "actorKill" });
    AppendSink<RE::BooksRead, RE::BooksRead::Event>(
      &std::vector{ "booksRead" });
    AppendSink<RE::CriticalHit, RE::CriticalHit::Event>(
      &std::vector{ "criticalHit" });
    AppendSink<RE::DisarmedEvent, RE::DisarmedEvent::Event>(
      &std::vector{ "disarmedEvent" });
    AppendSink<RE::DragonSoulsGained, RE::DragonSoulsGained::Event>(
      &std::vector{ "dragonSoulsGained" });
    AppendSink<RE::ItemHarvested, RE::ItemHarvested::Event>(
      &std::vector{ "itemHarvested" });
    AppendSink<RE::LevelIncrease, RE::LevelIncrease::Event>(
      &std::vector{ "levelIncrease" });
    AppendSink<RE::LocationDiscovery, RE::LocationDiscovery::Event>(
      &std::vector{ "locationDiscovery" });
    AppendSink<RE::ShoutAttack, RE::ShoutAttack::Event>(
      &std::vector{ "shoutAttack" });
    AppendSink<RE::SkillIncrease, RE::SkillIncrease::Event>(
      &std::vector{ "skillIncrease" });
    AppendSink<RE::SoulsTrapped, RE::SoulsTrapped::Event>(
      &std::vector{ "soulsTrapped" });
    AppendSink<RE::SpellsLearned, RE::SpellsLearned::Event>(
      &std::vector{ "spellsLearned" });
  };
  EventHandlerStory(const EventHandlerStory&) = delete;
  EventHandlerStory(EventHandlerStory&&) = delete;

  ~EventHandlerStory() = default;
};
