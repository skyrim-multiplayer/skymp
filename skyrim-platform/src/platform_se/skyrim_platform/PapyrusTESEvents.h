#pragma once
#include <RE/ScriptEventSourceHolder.h>
#include <RE/TESObjectREFR.h>

namespace PapyrusTESEvents {

struct TESSpellCastEvent
{
public:
  TESSpellCastEvent();
  TESSpellCastEvent(RE::TESObjectREFR* a_caster, RE::FormID a_spell);
  ~TESSpellCastEvent() = default;
  RE::NiPointer<RE::TESObjectREFR> caster;
  RE::FormID spell;
};
static_assert(sizeof(TESSpellCastEvent) == 0x10);
}
