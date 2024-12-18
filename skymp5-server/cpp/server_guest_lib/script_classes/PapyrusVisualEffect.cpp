#include "PapyrusVisualEffect.h"
#include "MpActor.h"
#include "SpSnippetFunctionGen.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusVisualEffect::Play(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Play", arguments);
  return VarValue::None();
}

VarValue PapyrusVisualEffect::Stop(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Stop", arguments);
  return VarValue::None();
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
      for (auto listener : actorForm->GetActorListeners()) {
        SpSnippet(
          GetName(), funcName,
          SpSnippetFunctionGen::SerializeArguments(arguments, listener).data(),
          selfRec.ToGlobalId(selfRec.rec->GetId()))
          .Execute(listener, SpSnippetMode::kNoReturnResult);
        // Workaround to use this function on player clone
        if (actorForm->GetFormId() == listener->GetFormId()) {
          SpSnippet(GetName(), funcName,
                    SpSnippetFunctionGen::SerializeArguments(arguments),
                    selfRec.ToGlobalId(selfRec.rec->GetId()))
            .Execute(listener, SpSnippetMode::kNoReturnResult);
        }
      }
    }
  } else {
    throw std::runtime_error(std::string(funcName) + ": can't get object!");
  }
}
