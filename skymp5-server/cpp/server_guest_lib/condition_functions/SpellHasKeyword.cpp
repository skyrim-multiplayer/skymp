#include "SpellHasKeyword.h"
#include "MpActor.h"

const char* ConditionFunctions::SpellHasKeyword::GetName() const
{
  return "SpellHasKeyword";
}

uint16_t ConditionFunctions::SpellHasKeyword::GetFunctionIndex() const
{
  return 596;
}

// https://ck.uesp.net/wiki/SpellHasKeyword
// https://ck.uesp.net/wiki/Casting_Source
float ConditionFunctions::SpellHasKeyword::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  // not really proven this order in espm is correct
  const uint32_t castingSource = parameter1;
  const uint32_t keywordId = parameter2;

  // same goes for these enum values
  constexpr uint32_t kCastingSourceLeft = 0;
  constexpr uint32_t kCastingSourceRight = 1;
  constexpr uint32_t kCastingSourceVoice = 2;
  constexpr uint32_t kCastingSourceInstant = 3;

  std::optional<uint32_t> spellToCheck;

  switch (castingSource) {
    case kCastingSourceLeft:
      spellToCheck = actor.GetEquipment().leftSpell;
      break;
    case kCastingSourceRight:
      spellToCheck = actor.GetEquipment().rightSpell;
      break;
    case kCastingSourceVoice:
      break; // Not yet supported
    case kCastingSourceInstant:
      break; // Not yet supported
  }

  if (!spellToCheck.has_value() || *spellToCheck == 0) {
    return 0.f;
  }

  auto worldState = actor.GetParent();
  auto& br = worldState->GetEspm().GetBrowser();

  espm::LookupResult spell = br.LookupById(*spellToCheck);
  if (!spell.rec) {
    return 0.f;
  }

  std::vector<uint32_t> keywordIds =
    spell.rec->GetKeywordIds(worldState->GetEspmCache());

  if (std::any_of(keywordIds.begin(), keywordIds.end(), [&](uint32_t id) {
        return spell.ToGlobalId(id) == keywordId;
      })) {
    return 1.f;
  }

  return 0.f;
}
