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
  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - parameter1={}, "
               "parameter2={}",
               parameter1, parameter2);

  // not really proven this order in espm is correct
  const uint32_t castingSource = parameter1;
  const uint32_t keywordId = parameter2;

  // same goes for these enum values
  constexpr uint32_t kCastingSourceLeft = 0;
  constexpr uint32_t kCastingSourceRight = 1;
  constexpr uint32_t kCastingSourceVoice = 2;
  constexpr uint32_t kCastingSourceInstant = 3;

  std::optional<uint32_t> spellToCheck;

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - "
               "castingSource={}, keywordId={:x}",
               castingSource, keywordId);

  switch (castingSource) {
    case kCastingSourceLeft:
      spellToCheck = actor.GetEquipment().leftSpell;
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - "
                   "kCastingSourceLeft, leftSpell={:x}",
                   spellToCheck.value_or(0));
      break;
    case kCastingSourceRight:
      spellToCheck = actor.GetEquipment().rightSpell;
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - "
                   "kCastingSourceRight, rightSpell={:x}",
                   spellToCheck.value_or(0));
      break;
    case kCastingSourceVoice:
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - "
                   "kCastingSourceVoice (not yet supported)");
      break; // Not yet supported
    case kCastingSourceInstant:
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - "
                   "kCastingSourceInstant (not yet supported)");
      break; // Not yet supported
    default:
      spdlog::warn("ConditionFunctions::SpellHasKeyword::Execute - unknown "
                   "castingSource={}",
                   castingSource);
      break;
  }

  if (!spellToCheck.has_value() || *spellToCheck == 0) {
    spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - no spell to "
                 "check, returning 0");
    return 0.f;
  }

  auto worldState = actor.GetParent();
  auto& espmCache = worldState->GetEspmCache();
  auto& br = worldState->GetEspm().GetBrowser();

  espm::LookupResult spellLookup = br.LookupById(*spellToCheck);
  if (!spellLookup.rec) {
    spdlog::warn("ConditionFunctions::SpellHasKeyword::Execute - spell record "
                 "not found for ID={:x}, returning 0",
                 *spellToCheck);
    return 0.f;
  }

  const espm::SPEL* spellRecord = espm::Convert<espm::SPEL>(spellLookup.rec);
  if (!spellRecord) {
    spdlog::warn("ConditionFunctions::SpellHasKeyword::Execute - record "
                 "{:x} is not a SPEL, returning 0",
                 *spellToCheck);
    return 0.f;
  }

  espm::SPEL::Data spellData = spellRecord->GetData(espmCache);

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - spell {:x} "
               "has {} effects",
               *spellToCheck, spellData.effects.size());

  // Check keywords on each magic effect of the spell
  for (const auto& effect : spellData.effects) {
    if (effect.effectFormId == 0) {
      continue;
    }

    uint32_t globalEffectId = spellLookup.ToGlobalId(effect.effectFormId);
    espm::LookupResult mgefLookup = br.LookupById(globalEffectId);
    if (!mgefLookup.rec) {
      spdlog::warn("ConditionFunctions::SpellHasKeyword::Execute - MGEF "
                   "record not found for ID={:x}",
                   globalEffectId);
      continue;
    }

    std::vector<uint32_t> mgefKeywordIds =
      mgefLookup.rec->GetKeywordIds(espmCache);
    spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - MGEF {:x} "
                 "has {} keywords",
                 globalEffectId, mgefKeywordIds.size());

    for (uint32_t localKeywordId : mgefKeywordIds) {
      uint32_t globalKeywordId = mgefLookup.ToGlobalId(localKeywordId);
      spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - checking "
                   "keyword local={:x}, global={:x} against target={:x}",
                   localKeywordId, globalKeywordId, keywordId);
      if (globalKeywordId == keywordId) {
        spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - keyword "
                     "match found on MGEF {:x}, returning 1",
                     globalEffectId);
        return 1.f;
      }
    }
  }

  spdlog::info("ConditionFunctions::SpellHasKeyword::Execute - no keyword "
               "match on any magic effect, returning 0");
  return 0.f;
}
