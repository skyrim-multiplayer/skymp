#include "BaseDescBinding.h"

Napi::Value BaseDescBinding::Get(Napi::Env env, ScampServer& scampServer,
                                 uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  auto desc =
    FormDesc::FromFormId(refr.GetBaseId(), partOne->worldState.espmFiles)
      .ToString();
  return Napi::String::New(env, desc);
}
