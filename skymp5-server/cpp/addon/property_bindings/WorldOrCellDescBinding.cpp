#include "WorldOrCellDescBinding.h"

Napi::Value WorldOrCellDescBinding::Get(Napi::Env env,
                                        ScampServer& scampServer,
                                        uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  auto desc = refr.GetCellOrWorld().ToString();
  return Napi::String::New(env, desc);
}
