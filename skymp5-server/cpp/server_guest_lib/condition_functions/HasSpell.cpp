#include "HasSpell.h"
#include "MpActor.h"
#include <algorithm>

const char* ConditionFunctions::HasSpell::GetName() const
{
  return "HasSpell";
}

uint16_t ConditionFunctions::HasSpell::GetFunctionIndex() const
{
  return 264;
}

float ConditionFunctions::HasSpell::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2)
{
  auto spelllist = actor.GetSpellList();
  auto it = std::find(spelllist.begin(), spelllist.end(), parameter1);
  if (it != spelllist.end()) {
    return 1.0f;
  }
  return 0.f;
}
