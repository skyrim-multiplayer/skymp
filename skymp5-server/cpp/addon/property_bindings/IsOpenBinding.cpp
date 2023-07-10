#include "IsOpenBinding.h"

Napi::Value IsOpenBinding::Get(Napi::Env env, ScampServer& scampServer,
                               uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  return Napi::Boolean::New(env, refr.IsOpen());
}

void IsOpenBinding::Set(Napi::Env env, ScampServer& scampServer,
                        uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  refr.SetOpen(newValue.As<Napi::Boolean>().Value());
}
