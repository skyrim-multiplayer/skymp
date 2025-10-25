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

#include "SkympGetIsWeaponHitSource.h"
#include "SkympGetWeaponHitSourceHasKeyword.h"

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
    std::make_shared<ConditionFunctions::WornApparelHasKeywordCount>());

  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympGetIsWeaponHitSource>());
  res.RegisterConditionFunction(
    std::make_shared<ConditionFunctions::SkympGetWeaponHitSourceHasKeyword>());

  return res;
}
