#include "PapyrusEffectShader.h"

#include "WorldState.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusEffectShader::Play(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Play", arguments);
  return VarValue::None();
}

VarValue PapyrusEffectShader::Stop(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  Helper(self, "Stop", arguments);
  return VarValue::None();
}

void PapyrusEffectShader::Helper(VarValue& self, const char* funcName,
                                 const std::vector<VarValue>& arguments)
{
  const auto& selfRec = GetRecordPtr(self);
  if (selfRec.rec) {
    if (arguments.size() < 1) {
      throw std::runtime_error(std::string(funcName) +
                               " requires at least one argument");
    }
    // TODO: Make normal sync for this. For now using workaround to inform
    // neigbours by sending papyrus functions to them.
    if (auto actorForm = GetFormPtr<MpObjectReference>(arguments[0])) {
      for (auto listener : actorForm->GetListeners()) {
        auto targetRefr = dynamic_cast<MpActor*>(listener);
        if (targetRefr) {
          SpSnippet(
            GetName(), funcName,
            SpSnippetFunctionGen::SerializeArguments(arguments, targetRefr)
              .data(),
            selfRec.ToGlobalId(selfRec.rec->GetId()))
            .Execute(targetRefr);
          // Workaround to use this function on player clone
          if (actorForm->GetFormId() == targetRefr->GetFormId()) {
            SpSnippet(
              GetName(), funcName,
              SpSnippetFunctionGen::SerializeArguments(arguments).data(),
              selfRec.ToGlobalId(selfRec.rec->GetId()))
              .Execute(targetRefr);
          }
        }
      }
    }
  } else {
    throw std::runtime_error(std::string(funcName) + ": can't get object!");
  }
}
