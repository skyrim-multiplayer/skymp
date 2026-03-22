#include "SkympGetDamageSourceHasKeyword.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympGetDamageSourceHasKeyword::GetName() const
{
  return "SkympGetDamageSourceHasKeyword";
}

uint16_t ConditionFunctions::SkympGetDamageSourceHasKeyword::GetFunctionIndex()
  const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympGetDamageSourceHasKeyword::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext& context)
{
  if (context.damageSourceFormId.has_value()) {
    auto& br = actor.GetParent()->GetEspm().GetBrowser();
    espm::LookupResult keyword = br.LookupById(parameter1);

    if (!keyword.rec) {
      spdlog::warn("SkympGetDamageSourceHasKeyword::Execute - keyword with "
                   "formId {:x} not found",
                   parameter1);
      return 0.f;
    }

    espm::LookupResult hitSource = br.LookupById(*context.damageSourceFormId);

    if (!hitSource.rec) {
      if (*context.damageSourceFormId != 0) {
        spdlog::warn("SkympGetDamageSourceHasKeyword::Execute - damage source "
                     "with formId {:x} not found",
                     *context.damageSourceFormId);
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
