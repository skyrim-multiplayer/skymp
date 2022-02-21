#pragma once

namespace RE {

struct ItemHarvested
{
public:
  struct Event
  {
  public:
    // members
    RE::TESBoundObject* produceItem; // 00
    RE::Actor* harvester;            // 08
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<ItemHarvested::Event>* GetEventSource()
  {
    using func_t = decltype(&ItemHarvested::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::ItemHarvested };
    return func();
  }
};

}
