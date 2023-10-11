#include "PapyrusSound.h"

#include "EspmGameObject.h"
#include "MpFormGameObject.h"
#include "MpObjectReference.h"
#include "SpSnippetFunctionGen.h"

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
    for (auto listener : refr->GetListeners()) {
      auto targetRefr = dynamic_cast<MpActor*>(listener);
      if (targetRefr) {
        SpSnippet(GetName(), funcName, serializedArgs.data(), selfId)
          .Execute(targetRefr);
      }
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
