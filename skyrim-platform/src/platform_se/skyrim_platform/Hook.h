#pragma once

class Handler;
struct HandlerInfoPerThread;

class Hook
{
public:
  Hook(std::string hookName_, std::string eventNameVariableName_,
       std::optional<std::string> succeededVariableName_);

  // javascript thread only
  uint32_t AddHandler(const std::shared_ptr<Handler>& handler);
  void RemoveHandler(const uint32_t& id);

  // Thread-safe, but it isn't too useful actually
  std::string GetName() const;

  // Hooks are set on game functions that are being called from multiple
  // threads. So Enter/Leave methods are thread-safe, but all private methods
  // are for Chakra thread only

  void Enter(uint32_t selfId, std::string& eventName);

  void Leave(bool succeeded);

private:
  void HandleEnter(DWORD owningThread, uint32_t selfId, std::string& eventName,
                   const Napi::Env& env);

  void PrepareContext(HandlerInfoPerThread& h, const Napi::Env& env);

  void ClearContextStorage(HandlerInfoPerThread& h, Napi::Env env);

  void HandleLeave(DWORD owningThread, bool succeeded, Napi::Env env);

  const std::string hookName;
  const std::string eventNameVariableName;
  const std::optional<std::string> succeededVariableName;
  std::set<DWORD> inProgressThreads;
  std::map<uint32_t, std::shared_ptr<Handler>> handlers;
  uint32_t hCounter = 0;
  std::atomic<int> addRemoveBlocker = 0;
};
