#include "IsOnlineBinding.h"

Napi::Value IsOnlineBinding::Get(Napi::Env env, ScampServer& scampServer,
                                 uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();
  try {
    auto& actor = partOne->worldState.Get<MpActor>(formId); 
    auto userId = partOne->serverState.UserByActor(&actor);
    if (userId != Networking::InvalidUserId) {
      return Napi::Boolean::New(env, true);
    }
  } catch (std::exception& e) {
    spdlog::error(e.what());
    return Napi::Boolean::New(env, false);
  }
}
