#include "IsOnlineBinding.h"

Napi::Value IsOnlineBinding::Get(Napi::Env env, ScampServer& scampServer,
                                 uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  if (auto actor = dynamic_cast<MpActor*>(&refr)) {
    auto userId = partOne->serverState.UserByActor(actor);
    if (userId != Networking::InvalidUserId) {
      return Napi::Boolean::New(env, true);
    }
  }
  return Napi::Boolean::New(env, false);
}
