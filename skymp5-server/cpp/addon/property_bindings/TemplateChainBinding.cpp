#include "TemplateChainBinding.h"

Napi::Value TemplateChainBinding::Get(Napi::Env env, ScampServer& scampServer,
                                      uint32_t formId)
{
  auto& partOne = scampServer.GetPartOne();

  auto& refr = partOne->worldState.GetFormAt<MpObjectReference>(formId);

  if (auto actor = refr.AsActor()) {
    auto& templateChain = actor->GetTemplateChain();

    auto array = Napi::Array::New(env, templateChain.size());

    for (int i = 0; i < static_cast<int>(templateChain.size()); ++i) {
      auto formId = templateChain[i].ToFormId(partOne->worldState.espmFiles);
      array.Set(i, Napi::Number::New(env, formId));
    }

    return array;
  }

  return env.Undefined();
}
