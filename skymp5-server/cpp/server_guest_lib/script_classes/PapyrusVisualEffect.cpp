#include "PapyrusVisualEffect.h"
#include "EspmGameObject.h"
#include "MpActor.h"
#include "MpFormGameObject.h"
#include "SpSnippetFunctionGen.h"

VarValue PapyrusVisualEffect::Play(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Play", arguments);
}

VarValue PapyrusVisualEffect::Stop(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Stop", arguments);
}

// This is exact copy of PapyrusEffectShader::Helper
void PapyrusVisualEffect::Helper(VarValue& self, const char* funcName,
                                 const std::vector<VarValue>& arguments)
{
  const auto& selfRec = GetRecordPtr(self);
  if (selfRec.rec) {
    if (arguments.size() < 1) {
      throw std::runtime_error(std::string(funcName) +
                               " requires at least one argument");
    }
    if (auto actorForm = GetFormPtr<MpObjectReference>(arguments[0])) {
      for (auto listener : actorForm->GetListeners()) {
        auto targetRefr = dynamic_cast<MpActor*>(listener);
        if (targetRefr) {
          SpSnippet(
            GetName(), funcName,
            SpSnippetFunctionGen::SerializeArguments(arguments, targetRefr)
              .data(),
            selfRec.ToGlobalId(selfRec.rec->GetId()))
            .Execute(targetRefr, SpSnippetMode::kNoReturnResult);
          // Workaround to use this function on player clone
          if (actorForm->GetFormId() == targetRefr->GetFormId()) {
            SpSnippet(
              GetName(), funcName,
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              selfRec.ToGlobalId(selfRec.rec->GetId()))
              .Execute(targetRefr, SpSnippetMode::kNoReturnResult);
          }
        }
      }
    }
  } else {
    throw std::runtime_error(std::string(funcName) + ": can't get object!");
  }
}
