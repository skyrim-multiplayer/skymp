#include "CanRespawnBinding.h"

Napi::Value CanRespawnBinding::Get(Napi::Env env, ScampServer& scampServer,
                                   uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  return Napi::Boolean::New(env, actor.GetCanRespawn());
}

void CanRespawnBinding::Set(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  actor.SetCanRespawn(newValue.As<Napi::Boolean>().Value());
}
