#pragma once

namespace RE {

struct BooksRead
{
public:
  struct Event
  {
  public:
    // members
    RE::TESObjectBOOK* book;  // 00
    bool unk08;               // 08
    std::uint8_t pad11{ 0 };  // 09
    std::uint16_t pad12{ 0 }; // 10
  };
  static_assert(sizeof(Event) == 0x10);

  static RE::BSTEventSource<BooksRead::Event>* GetEventSource()
  {
    using func_t = decltype(&BooksRead::GetEventSource);
    REL::Relocation<func_t> func{ Offsets::EventSource::BooksRead };
    return func();
  }
};

}
