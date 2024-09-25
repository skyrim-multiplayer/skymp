#pragma once

#include "NapiHelper.h"

namespace ConstEnumApi {
void Register(Napi::Env env, Napi::Object& exports,
              std::shared_ptr<JsEngine> jsEngine);
}
