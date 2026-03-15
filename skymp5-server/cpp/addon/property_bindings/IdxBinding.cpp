#include "IdxBinding.h"

Napi::Value IdxBinding::Get(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  return Napi::Number::New(env, refr.GetIdx());
}
