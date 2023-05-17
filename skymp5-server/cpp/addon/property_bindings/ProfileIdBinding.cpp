#pragma once
#include "ProfileIdBinding.h"

Napi::Value ProfileIdBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  try {
    auto& actor = partOne->worldState.Get<MpActor>(formId);
    return Napi::Number::New(env, actor.GetChangeForm().profileId);
  } catch (std::exception& e) {
    spdlog::error(e.what());
    return env.Undefined();
  }
}
