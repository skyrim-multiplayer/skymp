#pragma once

#include "RE/B/BSTEvent.h"

namespace RE {
struct TESHarvestedEvent
{
public:
  struct ItemHarvested
  {
  public:
    // members
    RE::TESBoundObject* produceItem; // 00
    RE::Actor* harvester;            // 08
  };
  static_assert(sizeof(ItemHarvested) == 0x10);

  static RE::BSTEventSource<TESHarvestedEvent::ItemHarvested>* GetEventSource()
  {
    using func_t = decltype(&TESHarvestedEvent::GetEventSource);
    REL::Relocation<func_t> func{ REL::ID(14875) };
    return func();
  }
};
}
