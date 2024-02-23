#include "TemplateChainBinding.h"

Napi::Value TemplateChainBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (auto actor = dynamic_cast<MpActor*>(&refr)) {
    auto chForm = actor->GetChangeForm();

    auto array = Napi::Array::New(env, chForm.templateChain.size());

    for (int i = 0; i < static_cast<int>(chForm.templateChain.size()); ++i) {
      auto formId =
        chForm.templateChain[i].ToFormId(partOne->worldState.espmFiles);
      array.Set(i, Napi::Number::New(env, formId));
    }

    return array;
  }

  return env.Undefined();
}
