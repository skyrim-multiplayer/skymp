#pragma once
#include "HookPattern.h"
#include "QuickJSHookEngine.h"
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

// Represents a single registered hook script with optional filtering
struct HookScriptEntry
{
  uint32_t qjsScriptId = 0; // ID in QuickJSHookEngine
  std::optional<HookPattern> pattern;
  std::optional<double> minSelfId;
  std::optional<double> maxSelfId;

  bool Matches(uint32_t selfId, const std::string& eventName) const;
};

class Hook
{
public:
  Hook(std::string hookName_, std::string eventNameVariableName_,
       std::optional<std::string> succeededVariableName_);

  // Add a hook by providing JavaScript source code.
  // The source must define enter(ctx) and/or leave(ctx) functions.
  // Returns a handle ID that can be used to remove the hook.
  uint32_t AddScript(const std::string& source,
                     std::optional<double> minSelfId,
                     std::optional<double> maxSelfId,
                     std::optional<HookPattern> pattern);

  void RemoveScript(uint32_t id);

  std::string GetName() const;

  // Thread-safe entry/exit points called from game hooks
  void Enter(uint32_t selfId, std::string& eventName);
  void Leave(bool succeeded);

private:
  const std::string hookName;
  const std::string eventNameVariableName;
  const std::optional<std::string> succeededVariableName;

  // QuickJS engine owns all compiled hook scripts for this hook
  std::unique_ptr<QuickJSHookEngine> qjsEngine;

  std::map<uint32_t, HookScriptEntry> scripts;
  uint32_t nextHandlerId = 0;

  // Prevent add/remove while executing hooks
  std::atomic<int> addRemoveBlocker = 0;

  // Per-thread invocation tracking for nested Enter/Leave pairs
  static std::mutex g_invocationsMutex;
  struct InvocationInfo
  {
    std::vector<uint32_t> matchedScriptIds;
  };
  static std::map<DWORD, std::vector<InvocationInfo>> g_invocations;
};
