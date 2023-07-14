#include "PapyrusUtility.h"

#include "WorldState.h"
#include <random>

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

static std::mt19937 kGenerator{ std::random_device{}() };

VarValue PapyrusUtility::RandomInt(
  VarValue self, const std::vector<VarValue>& arguments) const noexcept
{
  int32_t min = 0, max = 100;
  switch (arguments.size()) {
    case 0:
      break;
    case 1:
      min = static_cast<int32_t>(arguments[0].CastToInt());
      break;
    case 2:
      min = static_cast<int32_t>(arguments[0].CastToInt());
      max = static_cast<int32_t>(arguments[1].CastToInt());
      break;
    default:
      return VarValue::None();
  }
  std::uniform_int_distribution<> distribute{ min, max };
  return VarValue(distribute(kGenerator));
}

void PapyrusUtility::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)

{
  compatibilityPolicy = policy;

  AddStatic(vm, "Wait", &PapyrusUtility::Wait);
  AddStatic(vm, "RandomInt", &PapyrusUtility::RandomInt);
}
