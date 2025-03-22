#include "LastAnimEventBinding.h"

Napi::Value LastAnimEventBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (auto actor = refr.AsActor()) {
    auto animData = actor->GetLastAnimEvent();
    if (animData.has_value()) {
      return Napi::String::New(env, animData->animEventName);
    }
    return env.Null();
  }

  return env.Undefined();
}
