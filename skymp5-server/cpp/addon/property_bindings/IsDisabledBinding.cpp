#include "IsDisabledBinding.h"
#include "NapiHelper.h"

Napi::Value IsDisabledBinding::Get(Napi::Env env, ScampServer& scampServer,
                                   uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  return Napi::Boolean::New(env, refr.IsDisabled());
}

void IsDisabledBinding::Set(Napi::Env env, ScampServer& scampServer,
                            uint32_t formId, Napi::Value newValue)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);
  if (refr.GetFormId() < 0xff000000) {
    throw std::runtime_error("'isDisabled' is not usable for non-FF forms");
  }
  bool newValueBoolean = NapiHelper::ExtractBoolean(newValue, "newValue");
  newValueBoolean ? refr.Disable() : refr.Enable();
}
