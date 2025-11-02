#include "SkympGetWeaponHitSourceHasKeyword.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympGetWeaponHitSourceHasKeyword::GetName()
  const
{
  return "SkympGetWeaponHitSourceHasKeyword";
}

uint16_t
ConditionFunctions::SkympGetWeaponHitSourceHasKeyword::GetFunctionIndex() const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympGetWeaponHitSourceHasKeyword::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext& context)
{
  if (context.hitSourceFormId.has_value()) {
    auto& br = actor.GetParent()->GetEspm().GetBrowser();
    espm::LookupResult keyword = br.LookupById(parameter1);

    if (!keyword.rec) {
      spdlog::warn("SkympGetWeaponHitSourceHasKeyword::Execute - keyword with "
                   "formId {:x} not found",
                   parameter1);
      return 0.f;
    }

    espm::LookupResult hitSource = br.LookupById(*context.hitSourceFormId);

    if (!hitSource.rec) {
      if (*context.hitSourceFormId != 0) {
        spdlog::warn("SkympGetWeaponHitSourceHasKeyword::Execute - hit source "
                     "with formId {:x} not found",
                     *context.hitSourceFormId);
      }
      return 0.f;
    }

    std::vector<uint32_t> keywordIds =
      hitSource.rec->GetKeywordIds(actor.GetParent()->GetEspmCache());

    if (std::any_of(keywordIds.begin(), keywordIds.end(),
                    [&](uint32_t keywordId) {
                      return hitSource.ToGlobalId(keywordId) == parameter1;
                    })) {
      return 1.f;
    }
  }
  return 0.f;
}
