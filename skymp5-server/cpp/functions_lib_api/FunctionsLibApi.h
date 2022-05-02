#pragma once
#include "PartOne.h"
#include <JsEngine.h>

void RegisterFunctionsLibApi(std::shared_ptr<PartOne> partOne);

uint32_t Uint32FromJsValue(const JsValue& v);
