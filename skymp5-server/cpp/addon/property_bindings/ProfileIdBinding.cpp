#pragma once
#include "ProfileIdBinding.h"

Napi::Value ProfileIdBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (auto actor = dynamic_cast<MpActor*>(&refr)) {
    auto chForm = actor->GetChangeForm();
    return Napi::Number::New(env, chForm.profileId);
  }

  return env.Undefined();
}
