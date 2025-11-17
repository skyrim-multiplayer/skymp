#include "ConditionFunctionFactory.h"

#include "GetActorValuePercent.h"
#include "GetEquipped.h"
#include "GetIsRace.h"
#include "GetItemCount.h"
#include "HasSpell.h"
#include "IsWeaponMagicOut.h"
#include "IsWeaponOut.h"
#include "SpellHasKeyword.h"
#include "WornApparelHasKeywordCount.h"
#include "WornHasKeyword.h"

#include "GetEquippedItemType.h"
#include "IsBlocking.h"
#include "IsInInterior.h"

#include "SkympGetDamageSourceHasKeyword.h"
#include "SkympGetIsDamageSource.h"
#include "SkympWornExtendedApparelHasKeywordCount.h"
#include "SkympWornHasKeywordCount.h"

ConditionFunctionMap ConditionFunctionFactory::CreateConditionFunctions()
{
  ConditionFunctionMap res;

  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetEquipped>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetIsRace>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetItemCount>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::HasSpell>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::WornHasKeyword>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetActorValuePercent>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::IsWeaponMagicOut>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::IsWeaponOut>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SpellHasKeyword>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::WornApparelHasKeywordCount>()); // Repeats the features of the original condition - "does not count shields or amulets/necklaces"

  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::IsInInterior>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::GetEquippedItemType>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::IsBlocking>());

  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympGetDamageSourceHasKeyword>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympGetIsDamageSource>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympWornExtendedApparelHasKeywordCount>()); // Checks any form with the ARMO type
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympWornHasKeywordCount>()); // Checks any form

  return res;
}
