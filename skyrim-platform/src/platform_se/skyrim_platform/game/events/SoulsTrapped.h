#pragma once

namespace RE {

struct SoulsTrapped
{
public:
  struct Event
  {
  public:
    // members
    RE::Actor* trapper; // 00
    RE::Actor* target;  // 08
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<SoulsTrapped::Event>* GetEventSource()
  {
    using func_t = decltype(&SoulsTrapped::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::SoulsTrapped };
    return func();
  }

  static void SendEvent(RE::Actor* trapper, Actor* target)
  {
    Event e = { trapper, target };
    auto source = GetEventSource();
    if (source) {
      source->SendEvent(std::addressof(e));
    }
  }
};

}
