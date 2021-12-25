#pragma once
#include <RE/ScriptEventSourceHolder.h>
#include <RE/TESObjectREFR.h>

namespace TESEvents {

struct TESSpellCastEvent
{
public:
  TESSpellCastEvent();
  TESSpellCastEvent(RE::TESObjectREFR* caster, RE::FormID spell);
  ~TESSpellCastEvent() = default;
  RE::NiPointer<RE::TESObjectREFR> caster;
  RE::FormID spell;
};
static_assert(sizeof(TESSpellCastEvent) == 0x10);
}
