#include "OnlinePlayersBinding.h"

#include "Config.h"

Napi::Value OnlinePlayersBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t)
{
  auto& partOne = scampServer.GetPartOne();

  thread_local std::vector<uint32_t> g_onlineActors(kMaxPlayers);
  size_t numOnlineActors = 0;

  auto maxConnectedId = partOne->serverState.maxConnectedId;
  for (size_t i = 0; i <= maxConnectedId; ++i) {
    if (auto actor = partOne->serverState.ActorByUser(i)) {
      g_onlineActors[numOnlineActors] = actor->GetFormId();
      ++numOnlineActors;
    }
  }

  auto arr = Napi::Array::New(env, numOnlineActors);
  for (size_t k = 0; k < numOnlineActors; ++k) {
    auto id = g_onlineActors[k];
    arr.Set(k, Napi::Number::New(env, id));
  }
  return arr;
}
