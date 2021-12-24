#pragma once
#include <RE/ScriptEventSourceHolder.h>

namespace PapyrusSendEvent {
template <class T>
void Emit(T* event);
void TestEvent();
void Hit(RE::TESHitEvent* event);
void SpellCast(RE::TESSpellCastEvent* event);

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