#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterWeaponApi(std::shared_ptr<PartOne> partOne);

JsValue WeaponCtor(std::shared_ptr<PartOne> partOne,
                   const JsFunctionArguments& args);

JsValue GetBaseDamage(std::shared_ptr<PartOne> partOne,
                      const JsFunctionArguments& args);
