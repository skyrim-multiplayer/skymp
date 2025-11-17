#include "SkympWornExtendedApparelHasKeywordCount.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympWornExtendedApparelHasKeywordCount::GetName() const
{
  return "SkympWornExtendedApparelHasKeywordCount";
}

uint16_t ConditionFunctions::SkympWornExtendedApparelHasKeywordCount::GetFunctionIndex()
  const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympWornExtendedApparelHasKeywordCount::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  auto worldState = actor.GetParent();

  auto& equipment = actor.GetEquipment();

  size_t count = 0;

  for (auto& entry : equipment.inv.entries) {
    if (entry.GetWorn() == Inventory::Worn::None) {
      continue;
    }

    const espm::LookupResult res =
      worldState->GetEspm().GetBrowser().LookupById(entry.baseId);
    if (!res.rec) {
      continue;
    }
    if (!res.rec || res.rec->GetType() != espm::ARMO::kType) {
      continue;
    }
    
    std::vector<uint32_t> keywordIds =
      res.rec->GetKeywordIds(worldState->GetEspmCache());

    if (std::any_of(keywordIds.begin(), keywordIds.end(),
                    [&](uint32_t keywordId) {
                      return res.ToGlobalId(keywordId) == parameter1;
                    })) {
      ++count;
    }
  }

  return static_cast<float>(count);
}
