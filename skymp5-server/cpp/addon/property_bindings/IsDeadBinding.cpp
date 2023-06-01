#include "IsDeadBinding.h"

Napi::Value IsDeadBinding::Get(Napi::Env env, ScampServer& scampServer,
                               uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  return Napi::Boolean::New(env, actor.IsDead());
}

void IsDeadBinding::Set(Napi::Env env, ScampServer& scampServer,
                        uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  actor.SetIsDead(newValue.As<Napi::Boolean>().Value());
}
