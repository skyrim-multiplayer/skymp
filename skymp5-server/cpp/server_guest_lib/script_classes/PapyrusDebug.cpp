#include "PapyrusDebug.h"

#include "MpActor.h"
#include "SpSnippetFunctionGen.h"
#include "script_objects/MpFormGameObject.h"

VarValue PapyrusDebug::Notification(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "Notification",
                                       compatibilityPolicy, self, arguments);
}

VarValue PapyrusDebug::MessageBox(VarValue self,
                                  const std::vector<VarValue>& arguments)
{
  return ExecuteSpSnippetAndGetPromise(GetName(), "MessageBox",
                                       compatibilityPolicy, self, arguments);
}

VarValue PapyrusDebug::SendAnimationEvent(
  VarValue, const std::vector<VarValue>& arguments)
{
  auto targetActor = GetFormPtr<MpActor>(arguments[0]);
  if (targetActor) {
    auto funcName = "SendAnimationEvent";
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments, targetActor);
    SpSnippet(GetName(), funcName, s)
      .Execute(targetActor, SpSnippetMode::kNoReturnResult);
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

  AddStatic(vm, "Notification", &PapyrusDebug::Notification);
  AddStatic(vm, "MessageBox", &PapyrusDebug::MessageBox);
  AddStatic(vm, "SendAnimationEvent", &PapyrusDebug::SendAnimationEvent);
  AddStatic(vm, "Trace", &PapyrusDebug::Trace);
}
