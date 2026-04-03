#include "Hook.h"
#include "ScopedTask.h"
#include <spdlog/spdlog.h>
#include <Windows.h>

std::mutex Hook::g_invocationsMutex;
std::map<DWORD, std::vector<Hook::InvocationInfo>> Hook::g_invocations;

bool HookScriptEntry::Matches(uint32_t selfId,
                               const std::string& eventName) const
{
  if (minSelfId.has_value() && selfId < minSelfId.value()) {
    return false;
  }
  if (maxSelfId.has_value() && selfId > maxSelfId.value()) {
    return false;
  }
  if (pattern.has_value()) {
    switch (pattern->type) {
      case HookPatternType::Exact:
        return eventName == pattern->str;
      case HookPatternType::StartsWith:
        return eventName.size() >= pattern->str.size() &&
          !memcmp(eventName.data(), pattern->str.data(), pattern->str.size());
      case HookPatternType::EndsWith:
        return eventName.size() >= pattern->str.size() &&
          !memcmp(eventName.data() + (eventName.size() - pattern->str.size()),
                  pattern->str.data(), pattern->str.size());
    }
  }
  return true;
}

Hook::Hook(std::string hookName_, std::string eventNameVariableName_,
           std::optional<std::string> succeededVariableName_)
  : hookName(std::move(hookName_))
  , eventNameVariableName(std::move(eventNameVariableName_))
  , succeededVariableName(std::move(succeededVariableName_))
{
}

uint32_t Hook::AddScript(const std::string& source,
                         std::optional<double> minSelfId,
                         std::optional<double> maxSelfId,
                         std::optional<HookPattern> pattern)
{
  if (addRemoveBlocker) {
    throw std::runtime_error("Trying to add hook inside hook context");
  }

  std::string error;
  uint32_t qjsId = qjsEngine->Compile(source, hookName + "_hook", error);

  if (qjsId == 0) {
    throw std::runtime_error("Hook script compilation failed: " + error);
  }

  uint32_t handleId = nextHandlerId++;

  HookScriptEntry entry;
  entry.qjsScriptId = qjsId;
  entry.pattern = std::move(pattern);
  entry.minSelfId = minSelfId;
  entry.maxSelfId = maxSelfId;
  scripts[handleId] = std::move(entry);

  return handleId;
}

void Hook::RemoveScript(uint32_t id)
{
  if (addRemoveBlocker) {
    throw std::runtime_error("Trying to remove hook inside hook context");
  }

  auto it = scripts.find(id);
  if (it == scripts.end()) {
    return;
  }

  qjsEngine->RemoveScript(it->second.qjsScriptId);
  scripts.erase(it);
}

std::string Hook::GetName() const
{
  return hookName;
}

void Hook::Enter(uint32_t selfId, std::string& eventName)
{
  addRemoveBlocker++;
  Viet::ScopedTask<Hook> t([](Hook& hook) { hook.addRemoveBlocker--; }, *this);

  InvocationInfo invInfo;

  // Collect matching scripts and run enter
  for (auto& [handleId, entry] : scripts) {
    if (!entry.Matches(selfId, eventName)) {
      continue;
    }
    invInfo.matchedScriptIds.push_back(entry.qjsScriptId);

    auto result = qjsEngine->RunEnter(entry.qjsScriptId, selfId, eventName);
    if (!result.ok) {
      spdlog::error("Hook '{}' enter error: {}", hookName, result.error);
      continue;
    }
    eventName = result.eventName;
  }

  // Push invocation info for Leave to use
  const DWORD threadId = GetCurrentThreadId();
  {
    std::lock_guard l(g_invocationsMutex);
    g_invocations[threadId].push_back(std::move(invInfo));
  }
}

void Hook::Leave(bool succeeded)
{
  addRemoveBlocker++;
  Viet::ScopedTask<Hook> t([](Hook& hook) { hook.addRemoveBlocker--; }, *this);

  const DWORD threadId = GetCurrentThreadId();
  InvocationInfo invInfo;
  {
    std::lock_guard l(g_invocationsMutex);
    auto& stack = g_invocations[threadId];
    if (stack.empty()) {
      return;
    }
    invInfo = std::move(stack.back());
    stack.pop_back();
  }

  for (uint32_t scriptId : invInfo.matchedScriptIds) {
    auto result = qjsEngine->RunLeave(scriptId, succeeded);
    if (!result.ok) {
      spdlog::error("Hook '{}' leave error: {}", hookName, result.error);
    }
  }
}
