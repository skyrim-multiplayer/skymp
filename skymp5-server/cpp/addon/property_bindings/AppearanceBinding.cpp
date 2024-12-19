#include "AppearanceBinding.h"
#include "NapiHelper.h"
#include "UpdateAppearanceMessage.h"

Napi::Value AppearanceBinding::Get(Napi::Env env, ScampServer& scampServer,
                                   uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  auto& appearanceDump = actor.GetAppearanceAsJson();
  if (!appearanceDump.empty()) {
    return NapiHelper::ParseJson(env, appearanceDump);
  } else {
    return env.Null();
  }
}

void AppearanceBinding::Set(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId, Napi::Value newValue)
{
  // TODO: Validation
  auto& partOne = scampServer.GetPartOne();
  auto& actor = partOne->worldState.GetFormAt<MpActor>(formId);
  if (newValue.IsObject()) {
    auto appearanceDump = NapiHelper::Stringify(env, newValue);
    nlohmann::json j = nlohmann::json::parse(appearanceDump);
    auto appearance = Appearance::FromJson(j);
    actor.SetAppearance(&appearance);
  } else {
    actor.SetAppearance(nullptr);
  }

  constexpr int kChannelAppearance = 2;

  auto appearance = actor.GetAppearance();

  UpdateAppearanceMessage message;
  message.data =
    appearance ? std::optional<Appearance>(*appearance) : std::nullopt;
  message.idx = actor.GetIdx();

  for (auto listener : actor.GetActorListeners()) {
    // TODO: change to SendToUser, probably was deferred only for ability to
    // send text packets
    listener->SendToUserDeferred(message, true, kChannelAppearance, false);
  }
}
