#pragma once

#include "NapiHelper.h"

namespace ConstEnumApi {
void Register(Napi::Env env, Napi::Value& exports, std::shared_ptr<JsEngine> jsEngine);
}
