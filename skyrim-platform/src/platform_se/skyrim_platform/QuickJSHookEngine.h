#pragma once
#include "HooksStorage.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

// Lightweight JS engine for running hook scripts off the main Node.js thread.
// Each QuickJSHookEngine owns an independent QuickJS runtime+context with
// "hooksStorage" bindings already installed so scripts can share state with
// the Node.js side through HooksStorage.
class QuickJSHookEngine
{
public:
  QuickJSHookEngine();
  ~QuickJSHookEngine();

  QuickJSHookEngine(const QuickJSHookEngine&) = delete;
  QuickJSHookEngine& operator=(const QuickJSHookEngine&) = delete;

  // Compile a script once. Returns an opaque handle (0 on failure).
  // The error string is set on failure.
  uint32_t Compile(const std::string& source, const std::string& filename,
                   std::string& outError);

  // Run the enter callback for a compiled script.
  // Sets ctx.selfId, ctx.eventName (or the hook-specific variable name)
  // before calling the global enter() function.
  // Returns the (possibly modified) eventName.
  struct EnterResult
  {
    std::string eventName;
    bool ok = false;
    std::string error;
  };
  EnterResult RunEnter(uint32_t scriptId, uint32_t selfId,
                       const std::string& eventName);

  // Run the leave callback for a compiled script.
  // Sets ctx.succeeded (if applicable) before calling global leave().
  struct LeaveResult
  {
    bool ok = false;
    std::string error;
  };
  LeaveResult RunLeave(uint32_t scriptId, bool succeeded);

  void RemoveScript(uint32_t scriptId);

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};
