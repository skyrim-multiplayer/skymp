#pragma once

namespace RE {

struct CriticalHit
{
public:
  struct Event
  {
  public:
    // members
    RE::TESObjectREFR* aggressor; // 00
    RE::TESObjectWEAP* weapon;    // 08
    bool sneakHit;                // 10
    std::uint8_t pad11{ 0 };      // 11
    std::uint16_t pad12{ 0 };     // 12
    std::uint32_t pad14{ 0 };     // 14
  };
  static_assert(sizeof(Event) == 0x18);

  static RE::BSTEventSource<CriticalHit::Event>* GetEventSource()
  {
    using func_t = decltype(&CriticalHit::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::CriticalHit };
    return func();
  }
};

}
