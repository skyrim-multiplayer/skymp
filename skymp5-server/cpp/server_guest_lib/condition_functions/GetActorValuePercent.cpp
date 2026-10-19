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
  // AVIF form IDs follow the formula: 976 + espm::ActorValue enum index
  constexpr uint32_t kHealthActorValueId = 0x000003E8;      // enum 24
  constexpr uint32_t kMagickaActorValueId = 0x000003E9;     // enum 25
  constexpr uint32_t kStaminaActorValueId = 0x000003EA;     // enum 26
  constexpr uint32_t kHealRateActorValueId = 0x000003EB;    // enum 27
  constexpr uint32_t kMagickaRateActorValueId = 0x000003EC; // enum 28
  constexpr uint32_t kStaminaRateActorValueId = 0x000003ED; // enum 29
  constexpr uint32_t kHealRateMultActorValueId = 0x0000046B;    // enum 155
  constexpr uint32_t kMagickaRateMultActorValueId = 0x0000046C; // enum 156
  constexpr uint32_t kStaminaRateMultActorValueId = 0x0000046D; // enum 157

  auto& actorValues = actor.GetActorValues();

  switch (parameter1) {
    case kHealthActorValueId:
      return actorValues.healthPercentage;
    case kMagickaActorValueId:
      return actorValues.magickaPercentage;
    case kStaminaActorValueId:
      return actorValues.staminaPercentage;
    case kHealRateActorValueId:
      return actorValues.healRate;
    case kMagickaRateActorValueId:
      return actorValues.magickaRate;
    case kStaminaRateActorValueId:
      return actorValues.staminaRate;
    case kHealRateMultActorValueId:
      return actorValues.healRateMult;
    case kMagickaRateMultActorValueId:
      return actorValues.magickaRateMult;
    case kStaminaRateMultActorValueId:
      return actorValues.staminaRateMult;
    default:
      break;
  }

  return 0.f;
}
