#include "PapyrusTESEvents.h"

namespace PapyrusTESEvents {

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
