#include "AppearanceBinding.h"
#include "NapiHelper.h"

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

  std::string msg;
  msg += Networking::MinPacketId;
  msg += nlohmann::json{
    { "data",
      appearance ? nlohmann::json::parse(appearance->ToJson())
                 : nlohmann::json{} },
    { "idx", actor.GetIdx() },
    { "t", MsgType::UpdateAppearance }
  }.dump();

  for (auto listener : actor.GetListeners()) {
    auto listenerActor = dynamic_cast<MpActor*>(listener);
    if (listenerActor) {
      // TODO: change to SendToUser
      listenerActor->SendToUserDeferred(msg.data(), msg.size(), true,
                                        kChannelAppearance, false);
    }
  }
}
