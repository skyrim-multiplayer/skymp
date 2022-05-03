#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterArmorApi(std::shared_ptr<PartOne> partOne);

JsValue ArmorCtor(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);

JsValue GetArmorRating(std::shared_ptr<PartOne> partOne,
                       const JsFunctionArguments& args);

JsValue GetSlot(std::shared_ptr<PartOne> partOne,
                const JsFunctionArguments& args);
