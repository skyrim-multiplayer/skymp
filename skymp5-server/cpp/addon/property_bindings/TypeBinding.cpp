#include "TypeBinding.h"

Napi::Value TypeBinding::Get(Napi::Env env, ScampServer& scampServer,
                             uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (refr.AsActor()) {
    return Napi::String::New(env, "MpActor");
  } else {
    return Napi::String::New(env, "MpObjectReference");
  }
}
