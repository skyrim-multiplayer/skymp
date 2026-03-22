#include "SkympGetIsDamageSource.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympGetIsDamageSource::GetName() const
{
  return "SkympGetIsDamageSource";
}

uint16_t ConditionFunctions::SkympGetIsDamageSource::GetFunctionIndex() const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympGetIsDamageSource::Execute(
  MpActor&, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext& context)
{
  if (context.damageSourceFormId.has_value() &&
      *context.damageSourceFormId == parameter1) {
    return 1.0f;
  }
  return 0.f;
}
