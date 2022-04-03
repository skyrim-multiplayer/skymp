#pragma once

namespace RE {

struct DisarmedEvent
{
public:
  struct Event
  {
  public:
    // members
    RE::Actor* source; // 00
    RE::Actor* target; // 08
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<DisarmedEvent::Event>* GetEventSource()
  {
    using func_t = decltype(&DisarmedEvent::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::DisarmedEvent };
    return func();
  }
};

}
