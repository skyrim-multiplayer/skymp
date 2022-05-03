#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterLocationApi(std::shared_ptr<PartOne> partOne);

JsValue LocationCtor(std::shared_ptr<PartOne> partOne,
                     const JsFunctionArguments& args);

JsValue GetParent(std::shared_ptr<PartOne> partOne,
                  const JsFunctionArguments& args);
