#include "PapyrusUtility.h"

#include "WorldState.h"

VarValue PapyrusUtility::Wait(VarValue self,
                              const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1)
    throw std::runtime_error("Wait requires at least 1 argument");
  double seconds = static_cast<double>(arguments[0].CastToFloat());
  auto worldState = compatibilityPolicy->GetWorldState();
  if (!worldState)
    throw std::runtime_error("worldState not found");

  auto timerPromise = worldState->SetTimer(seconds);

  auto resultPromise = Viet::Promise<VarValue>();

  timerPromise
    .Then(
      [resultPromise](Viet::Void) { resultPromise.Resolve(VarValue::None()); })
    .Catch([resultPromise](const char* e) { resultPromise.Reject(e); });

  return VarValue(resultPromise);
}
