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
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  auto worldState = actor.GetParent();

  auto& equipment = actor.GetEquipment();

  size_t count = 0;

  constexpr uint32_t BOD_HAIR = 0x00000002;
  constexpr uint32_t BOD_BODY = 0x00000004;
  constexpr uint32_t BOD_HANDS = 0x00000008;
  constexpr uint32_t BOD_FEET = 0x00000080;
  
  constexpr uint32_t ARMOR_BITS = 
      BOD_FEET | BOD_HAIR | BOD_BODY | BOD_HANDS;

  for (auto& entry : equipment.inv.entries) {
    if (entry.GetWorn() == Inventory::Worn::None) {
      continue;
    }

    const espm::LookupResult res =
      worldState->GetEspm().GetBrowser().LookupById(entry.baseId);
    if (!res.rec || res.rec->GetType() != espm::ARMO::kType) {
      continue;
    }

    auto data = espm::GetData<espm::ARMO>(entry.baseId, worldState);
    
    bool isArmorPiece = false;
    if (data.hasBOD2 && (data.BOD2_flags & ARMOR_BITS)) {
        isArmorPiece = true;
    } else if (data.hasBODT && (data.BODT_flags & ARMOR_BITS)) {
        isArmorPiece = true;
    }

    if (!isArmorPiece) {
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
