#include "SpellHasKeyword.h"
#include "MpActor.h"
#include <spdlog/spdlog.h>

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
  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - parameter1={}, parameter2={}", parameter1, parameter2);
  
  // not really proven this order in espm is correct
  const uint32_t castingSource = parameter1;
  const uint32_t keywordId = parameter2;

  // same goes for these enum values
  constexpr uint32_t kCastingSourceLeft = 0;
  constexpr uint32_t kCastingSourceRight = 1;
  constexpr uint32_t kCastingSourceVoice = 2;
  constexpr uint32_t kCastingSourceInstant = 3;

  std::optional<uint32_t> spellToCheck;

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - castingSource={}, keywordId=0x{:X}", castingSource, keywordId);

  switch (castingSource) {
    case kCastingSourceLeft:
      spellToCheck = actor.GetEquipment().leftSpell;
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - kCastingSourceLeft, leftSpell=0x{:X}", spellToCheck.value_or(0));
      break;
    case kCastingSourceRight:
      spellToCheck = actor.GetEquipment().rightSpell;
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - kCastingSourceRight, rightSpell=0x{:X}", spellToCheck.value_or(0));
      break;
    case kCastingSourceVoice:
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - kCastingSourceVoice (not yet supported)");
      break; // Not yet supported
    case kCastingSourceInstant:
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - kCastingSourceInstant (not yet supported)");
      break; // Not yet supported
    default:
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - unknown castingSource={}", castingSource);
      break;
  }

  if (!spellToCheck.has_value() || *spellToCheck == 0) {
    spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - no spell to check, returning 0");
    return 0.f;
  }

  auto worldState = actor.GetParent();
  auto& br = worldState->GetEspm().GetBrowser();

  espm::LookupResult spell = br.LookupById(*spellToCheck);
  if (!spell.rec) {
    spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - spell record not found for ID=0x{:X}, returning 0", *spellToCheck);
    return 0.f;
  }

  std::vector<uint32_t> keywordIds =
    spell.rec->GetKeywordIds(worldState->GetEspmCache());

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - spell 0x{:X} has {} keywords", *spellToCheck, keywordIds.size());

  if (std::any_of(keywordIds.begin(), keywordIds.end(), [&](uint32_t id) {
        uint32_t globalId = spell.ToGlobalId(id);
        spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - checking keyword local=0x{:X}, global=0x{:X} against target=0x{:X}", id, globalId, keywordId);
        return globalId == keywordId;
      })) {
    spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - keyword match found, returning 1");
    return 1.f;
  }

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - no keyword match, returning 0");
  return 0.f;
}
