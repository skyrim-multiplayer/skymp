#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterCellApi(std::shared_ptr<PartOne> partOne);

JsValue CellCtor(std::shared_ptr<PartOne> partOne,
                 const JsFunctionArguments& args);

JsValue GetLocation(std::shared_ptr<PartOne> partOne,
                    const JsFunctionArguments& args);
