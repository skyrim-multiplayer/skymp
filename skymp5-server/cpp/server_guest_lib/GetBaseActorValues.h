#pragma once
#include <cstdint>

struct BaseActorValues
{
   
  float health = 100;
  float stamina = 100;
  float magicka = 100;
  float healRate = 100;
  float staminaRate = 100;
  float magickaRate = 100;
  float healRateMult = 100;
  float staminaRateMult = 100;
  float magickaRateMult = 100;

  using PropertiesVisitor =
    std::function<void(const char* propName, const char* jsonValue)>;

  std::string BaseActorValuesToString(float attribute)
  {
    std::string formattedAttribute = std::to_string(attribute);
    return formattedAttribute;
  }

  void VisitBaseActorValues(BaseActorValues& baseActorValues, const PropertiesVisitor& visitor)
  {
    visitor("health", BaseActorValuesToString(baseActorValues.health).c_str());
    visitor("stamina",
            BaseActorValuesToString(baseActorValues.stamina).c_str());
    visitor("magicka",
            BaseActorValuesToString(baseActorValues.magicka).c_str());
    visitor("healRate",
            BaseActorValuesToString(baseActorValues.healRate).c_str());
    visitor("staminaRate",
            BaseActorValuesToString(baseActorValues.staminaRate).c_str());
    visitor("magickaRate",
            BaseActorValuesToString(baseActorValues.magickaRate).c_str());
    visitor("healRateMult",
            BaseActorValuesToString(baseActorValues.healRateMult).c_str());
    visitor("staminaRateMult",
            BaseActorValuesToString(baseActorValues.staminaRateMult).c_str());
    visitor("magickaRateMult",
            BaseActorValuesToString(baseActorValues.magickaRateMult).c_str());
  }
};

inline BaseActorValues GetBaseActorValues(uint32_t baseId,
                                          uint32_t raceIdOverride)
{
  BaseActorValues baseActorValues;
  return baseActorValues;
}
