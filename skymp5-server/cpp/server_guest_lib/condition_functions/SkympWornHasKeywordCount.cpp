#include "SkympWornHasKeywordCount.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympWornHasKeywordCount::GetName() const
{
  return "SkympWornHasKeywordCount";
}

uint16_t ConditionFunctions::SkympWornHasKeywordCount::GetFunctionIndex() const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympWornHasKeywordCount::Execute(
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

    std::vector<uint32_t> keywordIds =
      res.rec->GetKeywordIds(worldState->GetEspmCache());

    if (!std::any_of(keywordIds.begin(), keywordIds.end(),
                     [&](uint32_t keywordId) {
                       return res.ToGlobalId(keywordId) == parameter1;
                     })) {
      continue;
    }

    // If non-zero, treat it as a mask for body parts to check
    if (parameter2 != 0) {

      // Only consider ARMO records for body part flag checking
      if (res.rec->GetType() != espm::ARMO::kType) {
        continue;
      }

      auto data = espm::GetData<espm::ARMO>(entry.baseId, worldState);

      uint32_t bodyPartFlags = 0;
      if (data.hasBOD2) {
        bodyPartFlags = data.BOD2_flags;
      } else if (data.hasBODT) {
        bodyPartFlags = data.BODT_flags;
      }

      if ((bodyPartFlags & parameter2) == 0) {
        continue;
      }
    }

    ++count;
  }

  return static_cast<float>(count);
}
