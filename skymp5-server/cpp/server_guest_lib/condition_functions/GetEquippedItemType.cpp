#include "GetEquippedItemType.h"
#include "MpActor.h"

const char* ConditionFunctions::GetEquippedItemType::GetName() const
{
  return "GetEquippedItemType";
}

uint16_t ConditionFunctions::GetEquippedItemType::GetFunctionIndex() const
{
  return 597;
}

// parameter1 is a hand (0=left, 1=right)
float ConditionFunctions::GetEquippedItemType::Execute(
  MpActor& actor, uint32_t parameter1, [[maybe_unused]] uint32_t parameter2,
  const ConditionEvaluatorContext&)
{
  // Slightly different from esp format, see:
  // https://ck.uesp.net/wiki/GetEquippedItemType_-_Actor
  static const float kMagicSpellTypeValue = 9.f;
  static const float kShieldTypeValue = 10.f;
  static const float kTorchTypeValue = 11.f;
  static const float kCrossbowTypeValue = 12.f;

  auto equippedWeapon = actor.GetEquippedWeapon();
  auto equippedScroll = actor.GetEquippedScroll();
  auto equippedLight = actor.GetEquippedLight();
  auto equippedShield = actor.GetEquippedShield();


  static const std::optional<Inventory::Entry> kHandNull = std::nullopt;

  // Check weapons
  const std::optional<Inventory::Entry>& hand =
    parameter1 < equippedWeapon.size() ? equippedWeapon[parameter1]
                                       : kHandNull;
  if (hand) {
    auto baseId = hand->baseId;
    auto& loader = actor.GetParent()->GetEspm();
    espm::LookupResult form = loader.GetBrowser().LookupById(baseId);
    if (form.rec) {
      auto weap = espm::Convert<espm::WEAP>(form.rec);
      if (weap) {
        espm::WEAP::Data weapData =
          weap->GetData(actor.GetParent()->GetEspmCache());
        espm::WEAP::AnimType animType = weapData.weapDNAM->animType;

        switch (animType) {
          case espm::WEAP::AnimType::Other:
          case espm::WEAP::AnimType::OneHandSword:
          case espm::WEAP::AnimType::OneHandDagger:
          case espm::WEAP::AnimType::OneHandAxe:
          case espm::WEAP::AnimType::OneHandMace:
          case espm::WEAP::AnimType::TwoHandSword:
          case espm::WEAP::AnimType::TwoHandAxe:
          case espm::WEAP::AnimType::Bow:
          case espm::WEAP::AnimType::Staff:
            return static_cast<float>(animType);
          case espm::WEAP::AnimType::Crossbow:
            return kCrossbowTypeValue;
        }
      }
    }
  }

  // Check scrolls
  const std::optional<Inventory::Entry>& scrollHand =
    parameter1 < equippedScroll.size() ? equippedScroll[parameter1]
                                       : kHandNull;

  if (scrollHand) {
    return kMagicSpellTypeValue;
  }

  // Check lights
  const std::optional<Inventory::Entry>& lightHand =
    parameter1 < equippedLight.size() ? equippedLight[parameter1] : kHandNull;

  if (lightHand) {
    return kTorchTypeValue;
  }

  // Check spells
  std::optional<uint32_t> leftSpell = actor.GetEquipment().leftSpell;
  std::optional<uint32_t> rightSpell = actor.GetEquipment().rightSpell;

  if (leftSpell && *leftSpell > 0 && parameter1 == 0) {
    return kMagicSpellTypeValue;
  }

  if (rightSpell && *rightSpell > 0 && parameter1 == 1) {
    return kMagicSpellTypeValue;
  }

  // Check shields
  const std::optional<Inventory::Entry>& shieldHand =
    parameter1 < equippedShield.size() ? equippedShield[parameter1]
                                       : kHandNull;

  if (shieldHand) {
    return kShieldTypeValue;
  }

  return 0.f;
}
