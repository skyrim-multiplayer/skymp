#include "WornApparelHasKeywordCount.h"
#include "MpActor.h"

const char* ConditionFunctions::WornApparelHasKeywordCount::GetName() const
{
  return "WornApparelHasKeywordCount";
}

uint16_t ConditionFunctions::WornApparelHasKeywordCount::GetFunctionIndex()
  const
{
  return 722;
}

float ConditionFunctions::WornApparelHasKeywordCount::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2)
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

    std::vector<uint32_t> keywordIds =
      res.rec->GetKeywordIds(worldState->GetEspmCache());

    if (std::find(keywordIds.begin(), keywordIds.end(), parameter1) !=
        keywordIds.end()) {
      ++count;
    }
  }

  return static_cast<float>(count);
}
