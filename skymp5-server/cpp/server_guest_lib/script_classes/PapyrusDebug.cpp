#include "PapyrusDebug.h"

#include "MpActor.h"
#include "papyrus-vm/VirtualMachine.h"
#include "script_objects/MpFormGameObject.h"
#include <chrono>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

VarValue PapyrusDebug::SendAnimationEvent(
  VarValue, const std::vector<VarValue>& arguments)
{
  auto targetActor = GetFormPtr<MpActor>(arguments[0]);
  if (targetActor) {
    auto funcName = "SendAnimationEvent";
    auto s = SpSnippetFunctionGen::SerializeArguments(arguments, targetActor);
    SpSnippet(GetName(), funcName, s.data())
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

VarValue PapyrusDebug::SpDumpStacks(VarValue self,
                                    const std::vector<VarValue>& arguments)
{
  auto curTime = std::chrono::steady_clock::now();
  auto curTimeSystem = std::chrono::system_clock::now();

  std::vector<std::shared_ptr<StackData>> stacks;
  stacks.reserve(StackData::activeStacks.size());
  for (const std::weak_ptr<StackData>& stack : StackData::activeStacks) {
    auto locked = stack.lock();
    if (locked != nullptr) {
      stacks.push_back(std::move(locked));
    }
  }

  std::sort(stacks.begin(), stacks.end(),
            [](const std::shared_ptr<StackData>& a,
               const std::shared_ptr<StackData>& b) {
              return a->lastExec < b->lastExec;
            });

  nlohmann::json jRoot;
  jRoot["curTimeMs"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                         curTime.time_since_epoch())
                         .count();

  std::vector<nlohmann::json> dumpedStackData;
  dumpedStackData.reserve(stacks.size());
  for (const auto& stack : stacks) {
    nlohmann::json jStack;
    jStack["id"] = stack->stackIdHolder.GetStackId();
    if (stack->tracing.enabled) {
      jStack["idTracing"] = fmt::format(
        "{}-{}", stack->stackIdHolder.GetStackId(), stack->tracing.traceId);
    }
    jStack["lastExecMs"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        stack->lastExec.time_since_epoch())
        .count();
    jStack["lastExecMsAgo"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(curTime -
                                                            stack->lastExec)
        .count();
    jStack["lastExecSystemMs"] =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        (curTimeSystem - (curTime - stack->lastExec)).time_since_epoch())
        .count();
    dumpedStackData.push_back(std::move(jStack));
  }
  jRoot["stacks"] = std::move(dumpedStackData);

  spdlog::info("{}", jRoot.dump(/*indent=*/2));

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
  AddStatic(vm, "SpDumpStacks", &PapyrusDebug::SpDumpStacks);
}
