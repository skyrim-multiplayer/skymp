#include "PapyrusUtility.h"

#include "TimeUtils.h"
#include "WorldState.h"
#include <chrono>
#include <random>

VarValue PapyrusUtility::Wait(VarValue self,
                              const std::vector<VarValue>& arguments)
{
  if (arguments.size() < 1)
    throw std::runtime_error("Wait requires at least 1 argument");
  double seconds = static_cast<double>(arguments[0].CastToFloat());
  auto worldState = compatibilityPolicy->GetWorldState();
  if (!worldState) {
    throw std::runtime_error("worldState not found");
  }

  auto time = Viet::TimeUtils::To<std::chrono::milliseconds>(seconds);
  auto timerPromise = worldState->SetTimer(time);
  auto resultPromise = Viet::Promise<VarValue>();

  timerPromise
    .Then(
      [resultPromise](Viet::Void) { resultPromise.Resolve(VarValue::None()); })
    .Catch([resultPromise](const char* e) { resultPromise.Reject(e); });

  return VarValue(resultPromise);
}

static std::mt19937 g_generator{ std::random_device{}() };

VarValue PapyrusUtility::RandomInt(VarValue self,
                                   const std::vector<VarValue>& arguments)
{
  int32_t min = static_cast<int32_t>(arguments[0].CastToInt());
  int32_t max = static_cast<int32_t>(arguments[1].CastToInt());
  std::uniform_int_distribution<> distribute{ min, max };
  return VarValue(distribute(g_generator));
}

VarValue PapyrusUtility::RandomFloat(VarValue self,
                                     const std::vector<VarValue>& arguments)
{
  double min = static_cast<double>(arguments[0].CastToFloat());
  double max = static_cast<double>(arguments[1].CastToFloat());
  std::uniform_real_distribution<> distribute{ min, max };
  return VarValue(distribute(g_generator));
}

void PapyrusUtility::Register(
  VirtualMachine& vm, std::shared_ptr<IPapyrusCompatibilityPolicy> policy)

{
  compatibilityPolicy = policy;

  AddStatic(vm, "Wait", &PapyrusUtility::Wait);
  AddStatic(vm, "RandomInt", &PapyrusUtility::RandomInt);
  AddStatic(vm, "RandomFloat", &PapyrusUtility::RandomFloat);
}
