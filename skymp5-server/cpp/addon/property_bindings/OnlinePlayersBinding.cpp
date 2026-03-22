#include "OnlinePlayersBinding.h"

#include "Config.h"

Napi::Value OnlinePlayersBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t)
{
  auto& partOne = scampServer.GetPartOne();

  thread_local std::vector<uint32_t> g_onlineActorsBuffer(kMaxPlayers);
  size_t numOnlineActors = 0;

  for (size_t i = 0, n = partOne->serverState.maxConnectedId; i <= n; ++i) {
    if (auto actor = partOne->serverState.ActorByUser(i)) {
      g_onlineActorsBuffer[numOnlineActors] = actor->GetFormId();
      ++numOnlineActors;
    }
  }

  auto arr = Napi::Array::New(env, numOnlineActors);
  for (size_t k = 0; k < numOnlineActors; ++k) {
    uint32_t id = g_onlineActorsBuffer[k];
    arr.Set(k, Napi::Number::New(env, id));
  }
  return arr;
}
