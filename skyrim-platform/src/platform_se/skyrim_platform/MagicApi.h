#pragma once

namespace MagicApi {

JsValue CastSpellImmediate(const JsFunctionArguments& args);
JsValue InterruptCast(const JsFunctionArguments& args);

JsValue GetAnimationVariablesFromActor(const JsFunctionArguments& args);
JsValue ApplyAnimationVariablesToActor(const JsFunctionArguments& args);

void Register(JsValue& exports);

}
