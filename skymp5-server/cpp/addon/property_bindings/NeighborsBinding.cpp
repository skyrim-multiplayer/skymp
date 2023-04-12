#include "NeighborsBinding.h"

Napi::Value NeighborsBinding::Get(Napi::Env env, ScampServer& scampServer,
                                  uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  std::set<MpObjectReference*> neighbors;

  for (auto listener : refr.GetListeners()) {
    neighbors.insert(listener);
  }
  for (auto emitter : refr.GetEmitters()) {
    neighbors.insert(emitter);
  }

  auto arr = Napi::Array::New(env, neighbors.size());
  int i = 0;
  for (auto id : neighbors) {
    arr.Set(i, Napi::Number::New(env, id->GetFormId()));
    ++i;
  }
  return arr;
}
