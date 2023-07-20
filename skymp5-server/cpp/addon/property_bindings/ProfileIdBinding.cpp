#include "ProfileIdBinding.h"

#include "NapiHelper.h"

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

void ProfileIdBinding::Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
           Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto newProfileId = NapiHelper::ExtractUInt32(newValue, "newProfileId");

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  if (auto actor = dynamic_cast<MpActor*>(&refr)) {
    actor->RegisterProfileId(static_cast<int32_t>(newProfileId));
  }
}
