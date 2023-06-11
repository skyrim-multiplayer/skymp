#include "PropertyBinding.h"

Napi::Value PropertyBinding::Get(Napi::Env, ScampServer&, uint32_t)
{
  throw std::runtime_error("mp.get is not implemented for '" +
                           GetPropertyName() + "'");
}

void PropertyBinding::Set(Napi::Env, ScampServer&, uint32_t, Napi::Value)
{
  throw std::runtime_error("mp.set is not implemented for '" +
                           GetPropertyName() + "'");
}
