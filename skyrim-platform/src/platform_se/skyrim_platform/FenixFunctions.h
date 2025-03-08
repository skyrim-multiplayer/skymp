#pragma once

namespace FenixFunctions {
JsValue ActorSit(const JsFunctionArguments& args);
JsValue ActorGetUp(const JsFunctionArguments& args);

void Register(JsValue& exports);
}
