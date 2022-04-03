#pragma once

namespace RE {

struct ShoutAttack
{
public:
  struct Event
  {
  public:
    // members
    RE::TESShout* shout; // 00
  };
  static_assert(sizeof(Event) == 0x08);

  static RE::BSTEventSource<ShoutAttack::Event>* GetEventSource()
  {
    using func_t = decltype(&ShoutAttack::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::ShoutAttack };
    return func();
  }
};

}
