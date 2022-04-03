#include "PapyrusDebug.h"

#include "MpActor.h"
#include "MpFormGameObject.h"

VarValue PapyrusDebug::SendAnimationEvent(
  VarValue self, const std::vector<VarValue>& arguments)
{
  auto targetActor = GetFormPtr<MpActor>(arguments[0]);
  if (targetActor) {
    auto funcName = "SendAnimationEvent";
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments, targetActor);
    SpSnippet(GetName(), funcName, s.data()).Execute(targetActor);
  }
  return VarValue::None();
}
