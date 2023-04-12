#include "ActorNeighborsBinding.h"

Napi::Value ActorNeighborsBinding::Get(Napi::Env env, ScampServer& scampServer,
                                       uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  std::set<MpActor*> neighbors;

  for (auto listener : refr.GetListeners()) {
    if (auto actor = dynamic_cast<MpActor*>(listener)) {
      neighbors.insert(actor);
    }
  }
  for (auto emitter : refr.GetEmitters()) {
    if (auto actor = dynamic_cast<MpActor*>(emitter)) {
      neighbors.insert(actor);
    }
  }

  auto arr = Napi::Array::New(env, neighbors.size());
  int i = 0;
  for (auto id : neighbors) {
    arr.Set(i, Napi::Number::New(env, id->GetFormId()));
    ++i;
  }
  return arr;
}
