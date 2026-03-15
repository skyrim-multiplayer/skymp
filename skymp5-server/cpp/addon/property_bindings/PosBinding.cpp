#include "PosBinding.h"

Napi::Value PosBinding::Get(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  auto niPoint3 = refr.GetPos();

  auto arr = Napi::Array::New(env, 3);
  arr.Set(uint32_t(0), Napi::Number::New(env, niPoint3.x));
  arr.Set(uint32_t(1), Napi::Number::New(env, niPoint3.y));
  arr.Set(uint32_t(2), Napi::Number::New(env, niPoint3.z));
  return arr;
}

void PosBinding::Set(Napi::Env env, ScampServer& scampServer, uint32_t formId,
                     Napi::Value newValue)
{
  throw std::runtime_error("mp.set is not implemented for '" +
                           GetPropertyName() +
                           "', use 'locationalData' instead");
}
