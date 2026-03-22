#include "GetActorValuePercent.h"
#include "MpActor.h"

const char* ConditionFunctions::GetActorValuePercent::GetName() const
{
  return "GetActorValuePercent";
}

uint16_t ConditionFunctions::GetActorValuePercent::GetFunctionIndex() const
{
  return 640;
}

float ConditionFunctions::GetActorValuePercent::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  constexpr uint32_t kHealthActorValueId = 0x000003E8;
  constexpr uint32_t kMagickaActorValueId = 0x000003E9;
  constexpr uint32_t kStaminaActorValueId = 0x000003EA;

  auto& actorValues = actor.GetActorValues();

  switch (parameter1) {
    case kHealthActorValueId:
      return actorValues.healthPercentage;
    case kMagickaActorValueId:
      return actorValues.magickaPercentage;
    case kStaminaActorValueId:
      return actorValues.staminaPercentage;
    default:
      break;
  }

  return 0.f;
}
