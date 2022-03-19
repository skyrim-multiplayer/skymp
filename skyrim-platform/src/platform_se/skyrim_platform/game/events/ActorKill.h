#pragma once

namespace RE {

struct ActorKill
{
public:
  struct Event
  {
  public:
    // members
    RE::Actor* killer; // 00
    RE::Actor* victim; // 08
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<ActorKill::Event>* GetEventSource()
  {
    using func_t = decltype(&ActorKill::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::ActorKill };
    return func();
  }
};

}
