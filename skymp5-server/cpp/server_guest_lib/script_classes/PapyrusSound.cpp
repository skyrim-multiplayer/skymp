#include "PapyrusSound.h"

#include "MpObjectReference.h"
#include "SpSnippetFunctionGen.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusSound::Play(VarValue self,
                            const std::vector<VarValue>& arguments)
{
  auto sound = GetRecordPtr(self);
  if (!sound.rec) {
    throw std::runtime_error("Self not found");
  }

  auto selfId = sound.ToGlobalId(sound.rec->GetId());

  if (auto refr = GetFormPtr<MpObjectReference>(arguments[0])) {
    if (arguments.size() < 1) {
      throw std::runtime_error("Play requires at least 1 argument");
    }
    auto funcName = "Play";
    auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
    for (auto listener : refr->GetActorListeners()) {
      SpSnippet(GetName(), funcName, serializedArgs, selfId)
        .Execute(listener, SpSnippetMode::kNoReturnResult);
    }
  }
  return VarValue::None();
}

void PapyrusSound::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddMethod(vm, "Play", &PapyrusSound::Play);
}
