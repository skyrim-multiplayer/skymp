#pragma once

namespace RE {

struct LocationDiscovery
{
public:
  struct Event
  {
  public:
    // members
    RE::MapMarkerData* mapMarkerData; // 00
    const char* worldspaceID;         // 08
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<LocationDiscovery::Event>* GetEventSource()
  {
    using func_t = decltype(&LocationDiscovery::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::LocationDiscovery };
    return func();
  }
};

}
