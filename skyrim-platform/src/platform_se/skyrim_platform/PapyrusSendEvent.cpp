#include "PapyrusSendEvent.h"
#include <RE/ScriptEventSourceHolder.h>

#include <RE/PlayerCharacter.h> // For test emmitter

namespace PapyrusSendEvent {
template <class T>
void Emit(T* event)
{
  auto sesh = RE::ScriptEventSourceHolder::GetSingleton();
  sesh->SendEvent<T>(event);
}

void Hit(RE::TESHitEvent* event)
{
  Emit<RE::TESHitEvent>(event);
}

void SpellCast(PapyrusSendEvent::TESSpellCastEvent* event)
{
  Emit<RE::TESSpellCastEvent>(reinterpret_cast<RE::TESSpellCastEvent*>(event));
}

void TestEvent()
{
  auto playerRef =
    static_cast<RE::TESObjectREFR*>(RE::PlayerCharacter::GetSingleton());
  // Test Hit
  /* Hit(new RE::TESHitEvent(playerRef, playerRef, static_cast<RE::FormID>(1),
                          static_cast<RE::FormID>(2),
                          RE::TESHitEvent::Flag::kHitBlocked));*/
  // Test SpellCast
  SpellCast(new TESSpellCastEvent(playerRef, static_cast<RE::FormID>(20)));
}

TESSpellCastEvent::TESSpellCastEvent()
  : TESSpellCastEvent(nullptr, static_cast<RE::FormID>(0))
{
}

TESSpellCastEvent::TESSpellCastEvent(RE::TESObjectREFR* a_caster,
                                     RE::FormID a_spell)
  : caster(a_caster)
  , spell(a_spell)
{
}

}
