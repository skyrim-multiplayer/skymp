#include "SkympGetIsWeaponHitSource.h"
#include "MpActor.h"

const char* ConditionFunctions::SkympGetIsWeaponHitSource::GetName() const
{
  return "SkympGetIsWeaponHitSource";
}

uint16_t ConditionFunctions::SkympGetIsWeaponHitSource::GetFunctionIndex()
  const
{
  return std::numeric_limits<uint16_t>::max();
}

float ConditionFunctions::SkympGetIsWeaponHitSource::Execute(
  MpActor&, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext& context)
{
  if (context.hitSourceFormId.has_value() &&
      *context.hitSourceFormId == parameter1) {
    return 1.0f;
  }
  return 0.f;
}
