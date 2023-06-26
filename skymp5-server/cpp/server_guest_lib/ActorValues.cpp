#include "ActorValues.h"
#include <fmt/format.h>

float ActorValues::GetValue(espm::ActorValue actorValue) const
{
  switch (actorValue) {
    case espm::ActorValue::Health:
      return health;
    case espm::ActorValue::Magicka:
      return magicka;
    case espm::ActorValue::Stamina:
      return stamina;
    case espm::ActorValue::HealRate:
      return healRate;
    case espm::ActorValue::MagickaRate:
      return magickaRate;
    case espm::ActorValue::StaminaRate:
      return staminaRate;
    case espm::ActorValue::HealRateMult_or_CombatHealthRegenMultMod:
      return healRateMult;
    case espm::ActorValue::MagickaRateMult_or_CombatHealthRegenMultPowerMod:
      return magickaRateMult;
    case espm::ActorValue::StaminaRateMult:
      return staminaRateMult;
    default:
      throw std::runtime_error(fmt::format("Unsupported actor value type {:}",
                                           static_cast<int32_t>(actorValue)));
  }
}
