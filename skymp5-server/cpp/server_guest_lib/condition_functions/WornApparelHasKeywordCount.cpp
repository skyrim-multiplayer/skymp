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

  constexpr uint32_t kBodHair = 0x00000002;
  constexpr uint32_t kBodBody = 0x00000004;
  constexpr uint32_t kBodHands = 0x00000008;
  constexpr uint32_t kBodFeet = 0x00000080;

  constexpr uint32_t kArmorBits = kBodFeet | kBodHair | kBodBody | kBodHands;

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

    bool isApparel = false;
    if (data.bod2.present && (data.bod2.bodyPartFlags & kArmorBits)) {
      isApparel = true;
    } else if (data.bodt.present && (data.bodt.bodyPartFlags & kArmorBits)) {
      isApparel = true;
    }

    if (!isApparel) {
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
