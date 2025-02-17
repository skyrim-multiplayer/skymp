#include "OnlinePlayersInfoBinding.h"

#include "Config.h"
#include <napi.h>

namespace {
struct PlayerInfo
{
  uint32_t actorId; // may be zero if not assigned to any actor
  std::string guid; // may be an empty string if
  Networking::UserId userId;
};
}

Napi::Value OnlinePlayersInfoBinding::Get(Napi::Env env,
                                          ScampServer& scampServer, uint32_t)
{
  auto& partOne = scampServer.GetPartOne();
  // const auto& serverState = partOne->serverState;

  thread_local std::vector<PlayerInfo> g_onlineActors(kMaxPlayers);
  size_t numOnlineActors = 0;

  auto maxConnectedId = partOne->serverState.maxConnectedId;
  for (size_t i = 0; i <= maxConnectedId; ++i) {
    if (auto actor = partOne->serverState.ActorByUser(i)) {
      g_onlineActors[numOnlineActors] = {
        actor->GetFormId(),
        partOne->serverState.UserGuid(i),
        static_cast<Networking::UserId>(i),
      };
      ++numOnlineActors;
    }
  }

  auto arr = Napi::Array::New(env, numOnlineActors);
  for (size_t k = 0; k < numOnlineActors; ++k) {
    const auto& info = g_onlineActors[k];
    auto obj = Napi::Object::New(env);
    obj.Set("actorId", info.actorId);
    obj.Set("guid", info.guid); // hide inside another 'networking' obj?
    obj.Set("userId", info.userId);
    arr.Set(k, obj);
  }
  return arr;
}
