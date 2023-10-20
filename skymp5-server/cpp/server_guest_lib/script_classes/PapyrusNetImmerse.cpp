#include "PapyrusNetImmerse.h"
#include "script_objects/EspmGameObject.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusNetImmerse::SetNodeTextureSet(
  VarValue, const std::vector<VarValue>& arguments)
{
  if (arguments.size() != 4) {
    throw std::runtime_error(
      "PapyrusNetImmerse::SetNodeTextureSet - expected 4 arguments");
  }

  MpObjectReference* ref = GetFormPtr<MpObjectReference>(arguments[0]);
  const char* node = static_cast<const char*>(arguments[1]);
  espm::LookupResult tSet = GetRecordPtr(arguments[2]);
  bool firstPerson = static_cast<bool>(arguments[3]);

  std::ignore = firstPerson;

  //   for (auto listener : selfRefr->GetListeners()) {
  //     auto targetRefr = dynamic_cast<MpActor*>(listener);
  //     if (targetRefr) {
  //       SpSnippet(GetName(), funcName, serializedArgs.data(),
  //                 selfRefr->GetFormId())
  //         .Execute(targetRefr);
  //     }
  //   }

  // TODO

  return VarValue::None();
}

VarValue PapyrusNetImmerse::SetNodeScale(
  VarValue, const std::vector<VarValue>& arguments)
{
  // TODO
  return VarValue::None();
}

void PapyrusNetImmerse::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddStatic(vm, "SetNodeTextureSet", &PapyrusNetImmerse::SetNodeTextureSet);
  AddStatic(vm, "SetNodeScale", &PapyrusNetImmerse::SetNodeScale);
}
