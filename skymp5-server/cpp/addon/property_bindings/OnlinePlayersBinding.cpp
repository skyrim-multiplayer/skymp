#include "OnlinePlayersBinding.h"

Napi::Value OnlinePlayersBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t)
{
  auto& partOne = scampServer.GetPartOne();

  auto n = partOne->serverState.userInfo.size();
  std::vector<uint32_t> onlineActors;
  onlineActors.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    if (auto actor = partOne->serverState.ActorByUser(i)) {
      onlineActors.push_back(actor->GetFormId());
    }
  }

  auto arr = Napi::Array::New(env, onlineActors.size());
  int i = 0;
  for (auto id : onlineActors) {
    arr.Set(i, Napi::Number::New(env, id));
    ++i;
  }
  return arr;
}
