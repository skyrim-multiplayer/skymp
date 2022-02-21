#pragma once

namespace RE {

struct DragonSoulsGained
{
public:
  struct Event
  {
  public:
    // members
    float souls;         // 00
    std::uint32_t pad04; // 04
  };
  static_assert(sizeof(Event) == 0x08);

  static RE::BSTEventSource<DragonSoulsGained::Event>* GetEventSource()
  {
    using func_t = decltype(&DragonSoulsGained::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::DragonSoulsGained };
    return func();
  }
};

}
