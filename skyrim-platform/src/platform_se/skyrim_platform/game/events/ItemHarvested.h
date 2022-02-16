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
#ifdef SKYRIMSE
    REL::Relocation<func_t> func{ REL::ID(14704) };
#else
    REL::Relocation<func_t> func{ REL::ID(14875) };
#endif
    return func();
  }
};

}
