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

  if (!ref) {
    throw std::runtime_error(
      "PapyrusNetImmerse::SetNodeTextureSet - ref is nullptr");
  }

  if (!tSet.rec) {
    throw std::runtime_error(
      "PapyrusNetImmerse::SetNodeTextureSet - tSet.rec is nullptr");
  }

  ref->SetNodeTextureSet(node, tSet, firstPerson);

  auto funcName = "SetNodeTextureSet";
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  for (auto listener : ref->GetListeners()) {
    auto targetRefr = dynamic_cast<MpActor*>(listener);
    if (targetRefr) {
      SpSnippet(GetName(), funcName, serializedArgs.data())
        .Execute(targetRefr);
    }
  }

  return VarValue::None();
}

VarValue PapyrusNetImmerse::SetNodeScale(
  VarValue, const std::vector<VarValue>& arguments)
{
  if (arguments.size() != 4) {
    throw std::runtime_error(
      "PapyrusNetImmerse::SetNodeScale - expected 4 arguments");
  }

  MpObjectReference* ref = GetFormPtr<MpObjectReference>(arguments[0]);
  const char* node = static_cast<const char*>(arguments[1]);
  float scale = static_cast<float>(static_cast<double>(arguments[2]));
  bool firstPerson = static_cast<bool>(arguments[3]);

  std::ignore = firstPerson;

  if (!ref) {
    throw std::runtime_error(
      "PapyrusNetImmerse::SetNodeScale - ref is nullptr");
  }

  ref->SetNodeScale(node, scale, firstPerson);

  auto funcName = "SetNodeScale";
  auto serializedArgs = SpSnippetFunctionGen::SerializeArguments(arguments);
  for (auto listener : ref->GetListeners()) {
    auto targetRefr = dynamic_cast<MpActor*>(listener);
    if (targetRefr) {
      SpSnippet(GetName(), funcName, serializedArgs.data())
        .Execute(targetRefr);
    }
  }

  return VarValue::None();
}

void PapyrusNetImmerse::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddStatic(vm, "setNodeTextureSet", &PapyrusNetImmerse::SetNodeTextureSet);
  AddStatic(vm, "setNodeScale", &PapyrusNetImmerse::SetNodeScale);
}
