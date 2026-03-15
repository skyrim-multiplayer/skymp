#include "ProfileIdBinding.h"

#include "NapiHelper.h"

Napi::Value ProfileIdBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (auto actor = refr.AsActor()) {
    return Napi::Number::New(env, actor->GetProfileId());
  }

  return env.Undefined();
}

void ProfileIdBinding::Set(Napi::Env env, ScampServer& scampServer,
                           uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto newProfileId = NapiHelper::ExtractUInt32(newValue, "newProfileId");

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  if (auto actor = refr.AsActor()) {
    actor->RegisterProfileId(static_cast<int32_t>(newProfileId));
  }
}
