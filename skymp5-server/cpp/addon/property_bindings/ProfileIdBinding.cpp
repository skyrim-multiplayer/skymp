#pragma once
#include "ProfileIdBinding.h"

Napi::Value ProfileIdBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  auto actor = partOne->worldState.Get<MpActor>(formId);
  return actor ? Napi::Number::New(env, actor->GetChangeForm().profileId)
               : env.Undefined();
}
