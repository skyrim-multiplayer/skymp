#include "TypeBinding.h"

Napi::Value TypeBinding::Get(Napi::Env env, ScampServer& scampServer,
                             uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  auto actor  = partOne->worldState.Get<MpActor>(formId);
  return actor ? Napi::String::New(env, "MpActor")
               : Napi::String::New(env, "MpObjectReference");
}
