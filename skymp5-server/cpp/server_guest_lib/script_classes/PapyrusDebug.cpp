#include "PapyrusDebug.h"

#include "MpActor.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusDebug::SendAnimationEvent(
  VarValue, const std::vector<VarValue>& arguments)
{
  auto targetActor = GetFormPtr<MpActor>(arguments[0]);
  if (targetActor) {
    auto funcName = "SendAnimationEvent";
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments, targetActor);
    SpSnippet(GetName(), funcName, s.data()).Execute(targetActor);
  }
  return VarValue::None();
}

VarValue PapyrusDebug::Trace(VarValue, const std::vector<VarValue>& arguments)
{
  const char* asTextToPrint = static_cast<const char*>(arguments[0]);
  int aiSeverity = static_cast<int>(arguments[1]);

  std::ignore = aiSeverity;

  spdlog::info("{}", asTextToPrint);

  return VarValue::None();
}

void PapyrusDebug::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)
{
  compatibilityPolicy = policy;

  AddStatic(vm, "notification", &PapyrusDebug::Notification);
  AddStatic(vm, "messageBox", &PapyrusDebug::MessageBox);
  AddStatic(vm, "sendAnimationEvent", &PapyrusDebug::SendAnimationEvent);
  AddStatic(vm, "trace", &PapyrusDebug::Trace);
}
