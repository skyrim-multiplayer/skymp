#include "TESEvents.h"

namespace TESEvents {

TESSpellCastEvent::TESSpellCastEvent()
  : TESSpellCastEvent(nullptr, static_cast<RE::FormID>(0))
{
}

TESSpellCastEvent::TESSpellCastEvent(RE::TESObjectREFR* caster,
                                     RE::FormID spell)
  : caster(caster)
  , spell(spell)
{
}

}
