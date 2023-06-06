#include "EventsApi.h"
#include "EventManager.h"
#include "ExceptionPrinter.h"
#include "IPC.h"
#include "InvalidArgumentException.h"
#include "JsUtils.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"

namespace {
enum class PatternType
{
  Exact,
  StartsWith,
  EndsWith
};

class Pattern
{
public:
  static Pattern Parse(const std::string& str)
  {
    auto count = std::count(str.begin(), str.end(), '*');
    if (count == 0) {
      return { PatternType::Exact, str };
    }
    if (count > 1) {
      throw std::runtime_error(
        "Patterns can contain only one '*' at the beginning/end of string");
    }

    auto pos = str.find('*');
    if (pos == 0) {
      return { PatternType::EndsWith,
               std::string(str.begin() + 1, str.end()) };
    }
    if (pos == str.size() - 1) {
      return { PatternType::StartsWith,
               std::string(str.begin(), str.end() - 1) };
    }
    throw std::runtime_error(
      "In patterns '*' must be at the beginning/end of string");
  }

  PatternType type;
  std::string str;
};

class Handler
{
public:
  Handler() = default;

  Handler(const JsValue& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_, std::optional<Pattern> pattern_)
    : enter(handler_.GetProperty("enter"))
    , leave(handler_.GetProperty("leave"))
    , minSelfId(minSelfId_)
    , maxSelfId(maxSelfId_)
    , pattern(pattern_)
  {
  }

  bool Matches(uint32_t selfId, const std::string& eventName)
  {
    if (minSelfId.has_value() && selfId < minSelfId.value()) {
      return false;
    }
    if (maxSelfId.has_value() && selfId > maxSelfId.value()) {
      return false;
    }
    if (pattern.has_value()) {
      switch (pattern->type) {
        case PatternType::Exact:
          return eventName == pattern->str;
        case PatternType::StartsWith:
          return eventName.size() >= pattern->str.size() &&
            !memcmp(eventName.data(), pattern->str.data(),
                    pattern->str.size());
        case PatternType::EndsWith:
          return eventName.size() >= pattern->str.size() &&
            !memcmp(eventName.data() +
                      (eventName.size() - pattern->str.size()),
                    pattern->str.data(), pattern->str.size());
      }
    }
    return true;
  }

  // PerThread structure is unique for each thread
  struct PerThread
  {
    JsValue storage, context;
    bool matchesCondition = false;
  };
  std::unordered_map<DWORD, PerThread> perThread;

  // Shared between threads
  const JsValue enter, leave;
  const std::optional<Pattern> pattern;
  const std::optional<double> minSelfId;
  const std::optional<double> maxSelfId;
};

class Hook
{
public:
  Hook(std::string hookName_, std::string eventNameVariableName_,
       std::optional<std::string> succeededVariableName_)
    : hookName(hookName_)
    , eventNameVariableName(eventNameVariableName_)
    , succeededVariableName(succeededVariableName_)
  {
  }

  // Chakra thread only
  uint32_t AddHandler(const Handler& handler)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to add hook inside hook context");
    }
    handlers.emplace(hCounter, handler);
    return hCounter++;
  }

  void RemoveHandler(const uint32_t& id)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to remove hook inside hook context");
    }
    handlers.erase(id);
  }

  // Thread-safe, but it isn't too useful actually
  std::string GetName() const { return hookName; }

  // Hooks are set on game functions that are being called from multiple
  // threads. So Enter/Leave methods are thread-safe, but all private methods
  // are for Chakra thread only

  void Enter(uint32_t selfId, std::string& eventName)
  {
    addRemoveBlocker++;
    DWORD owningThread = GetCurrentThreadId();

    if (hookName == "sendPapyrusEvent") {
      // If there are no handlers, do not do g_taskQueue
      bool anyMatch = false;
      for (auto& hp : handlers) {
        auto* h = &hp.second;
        if (h->Matches(selfId, eventName)) {
          anyMatch = true;
          break;
        }
      }
      if (!anyMatch) {
        return;
      }

      return SkyrimPlatform::GetSingleton()->AddUpdateTask(
        [this, owningThread, selfId, eventName] {
          std::string s = eventName;
          HandleEnter(owningThread, selfId, s);
        });
    }

    auto f = [&] {
      try {
        if (inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is already processing");
        inProgressThreads.insert(owningThread);
        HandleEnter(owningThread, selfId, eventName);
      } catch (std::exception& e) {
        auto err = std::string(e.what()) + " (while performing enter on '" +
          hookName + "')";
        SkyrimPlatform::GetSingleton()->AddUpdateTask(
          [err] { throw std::runtime_error(err); });
      }
    };
    SkyrimPlatform::GetSingleton()->PushAndWait(f);
    addRemoveBlocker--;
  }

  void Leave(bool succeeded)
  {
    addRemoveBlocker++;
    DWORD owningThread = GetCurrentThreadId();

    if (hookName == "sendPapyrusEvent") {
      return;
    }

    auto f = [&] {
      try {
        if (!inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is not processing");
        inProgressThreads.erase(owningThread);
        HandleLeave(owningThread, succeeded);

      } catch (std::exception& e) {
        std::string what = e.what();
        SkyrimPlatform::GetSingleton()->AddUpdateTask([what] {
          throw std::runtime_error(what + " (in SendAnimationEventLeave)");
        });
      }
    };
    SkyrimPlatform::GetSingleton()->PushAndWait(f);
    addRemoveBlocker--;
  }

private:
  void HandleEnter(DWORD owningThread, uint32_t selfId, std::string& eventName)
  {
    for (auto& hp : handlers) {
      auto* h = &hp.second;
      auto& perThread = h->perThread[owningThread];
      perThread.matchesCondition = h->Matches(selfId, eventName);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread);
      ClearContextStorage(perThread);

      perThread.context.SetProperty("selfId", static_cast<double>(selfId));
      perThread.context.SetProperty(eventNameVariableName, eventName);
      h->enter.Call({ JsValue::Undefined(), perThread.context });

      eventName = static_cast<std::string>(
        perThread.context.GetProperty(eventNameVariableName));
    }
  }

  void PrepareContext(Handler::PerThread& h)
  {
    if (h.context.GetType() != JsValue::Type::Object) {
      h.context = JsValue::Object();
    }

    thread_local auto g_standardMap =
      JsValue::GlobalObject().GetProperty("Map");
    if (h.storage.GetType() != JsValue::Type::Object) {
      h.storage = g_standardMap.Constructor({ g_standardMap });
      h.context.SetProperty("storage", h.storage);
    }
  }

  void ClearContextStorage(Handler::PerThread& h)
  {
    thread_local auto g_standardMap =
      JsValue::GlobalObject().GetProperty("Map");
    thread_local auto g_clear =
      g_standardMap.GetProperty("prototype").GetProperty("clear");
    g_clear.Call({ h.storage });
  }

  void HandleLeave(DWORD owningThread, bool succeeded)
  {
    for (auto& hp : handlers) {
      auto* h = &hp.second;
      auto& perThread = h->perThread.at(owningThread);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread);

      if (succeededVariableName.has_value()) {
        perThread.context.SetProperty(succeededVariableName.value(),
                                      JsValue::Bool(succeeded));
      }
      h->leave.Call({ JsValue::Undefined(), perThread.context });
      h->perThread.erase(owningThread);
    }
  }

  const std::string hookName;
  const std::string eventNameVariableName;
  const std::optional<std::string> succeededVariableName;
  std::set<DWORD> inProgressThreads;
  std::map<uint32_t, Handler> handlers;
  uint32_t hCounter = 0;
  std::atomic<int> addRemoveBlocker = 0;
};
}

struct EventsGlobalState
{
  EventsGlobalState()
  {
    sendAnimationEvent.reset(
      new Hook("sendAnimationEvent", "animEventName", "animationSucceeded"));
    sendPapyrusEvent.reset(
      new Hook("sendPapyrusEvent", "papyrusEventName", std::nullopt));
  }

  using Callbacks = std::map<std::string, std::vector<JsValue>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;
  std::shared_ptr<Hook> sendAnimationEvent;
  std::shared_ptr<Hook> sendPapyrusEvent;
} g;

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  auto manager = EventManager::GetSingleton();

  auto cbObjMap = manager->GetCallbackObjMap(eventName);

  if (!cbObjMap || cbObjMap->empty()) {
    logger::trace("Failed to retrieve callback map or the map is empty.");
    return;
  }

  std::vector<uintptr_t> callbacksToUnsubscribe;
  std::vector<CallbackObject> callbacksToCall;

  // 1. Collect all callbacks and remember "runOnce" callbacks
  for (const auto& [uid, callbackInfo] : *cbObjMap) {
    callbacksToCall.push_back(callbackInfo);
    if (callbackInfo.runOnce) {
      callbacksToUnsubscribe.push_back(uid);
    }
  }

  // 2. Make sure that "runOnce" callbacks will never be called again
  for (auto uid : callbacksToUnsubscribe) {
    manager->Unsubscribe(uid, eventName);
  }

  // 3. Finally, call the callbacks
  for (auto& callbackInfo : callbacksToCall) {
    try {
      callbackInfo.callback.Call(arguments);
    } catch (const std::exception& e) {
      const char* method = callbackInfo.runOnce ? "once" : "on";
      std::string what = e.what();
      logger::error("{}('{}'): {}", method, eventName, what);

      // We still write to the game console as we were doing before spdlog
      // integration. When I write something wrong when coding skymp5-client I
      // expect to see errors in game console
      ExceptionPrinter::Print(e);
    }
  }

  // You may want to optimize it. Feel free to. But remember that changing
  // order of those three steps may and WILL break user code like:
  //
  // sp.on("tick", () => {
  //   sp.once("tick", () => {
  //     sp.once("tick", () => {
  // // ...
}

void EventsApi::Clear()
{
  g = {};

  EventManager::GetSingleton()->ClearCallbacks();
}

void EventsApi::SendAnimationEventEnter(uint32_t selfId,
                                        std::string& animEventName) noexcept
{
  g.sendAnimationEvent->Enter(selfId, animEventName);
}

void EventsApi::SendAnimationEventLeave(bool animationSucceeded) noexcept
{
  g.sendAnimationEvent->Leave(animationSucceeded);
}

void EventsApi::SendPapyrusEventEnter(uint32_t selfId,
                                      std::string& papyrusEventName) noexcept
{
  g.sendPapyrusEvent->Enter(selfId, papyrusEventName);
}

void EventsApi::SendPapyrusEventLeave() noexcept
{
  g.sendPapyrusEvent->Leave(true);
}

namespace {
JsValue CreateHookApi(std::shared_ptr<Hook> hookInfo)
{
  auto hook = JsValue::Object();
  hook.SetProperty(
    "add", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      auto handlerObj = args[1];

      std::optional<double> minSelfId;
      if (args[2].GetType() == JsValue::Type::Number) {
        minSelfId = static_cast<double>(args[2]);
      }

      std::optional<double> maxSelfId;
      if (args[3].GetType() == JsValue::Type::Number) {
        maxSelfId = static_cast<double>(args[3]);
      }

      std::optional<Pattern> pattern;
      if (args[4].GetType() == JsValue::Type::String) {
        pattern = Pattern::Parse(static_cast<std::string>(args[4]));
      }

      Handler handler(handlerObj, minSelfId, maxSelfId, pattern);
      uint32_t id = hookInfo->AddHandler(handler);

      return JsValue((int)id);
    }));

  hook.SetProperty(
    "remove", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      uint32_t toRemove = static_cast<int>(args[1]);
      hookInfo->RemoveHandler(toRemove);
      return JsValue::Undefined();
    }));
  return hook;
}
}

JsValue EventsApi::GetHooks()
{
  auto res = JsValue::Object();
  for (auto& hook : { g.sendAnimationEvent, g.sendPapyrusEvent }) {
    res.SetProperty(hook->GetName(), CreateHookApi(hook));
  }
  return res;
}

namespace {
JsValue Subscribe(const JsFunctionArguments& args, bool runOnce = false)
{
  auto eventName = args[1].ToString();
  auto callback = args[2];

  auto handle =
    EventManager::GetSingleton()->Subscribe(eventName, callback, runOnce);

  auto obj = JsValue::Object();
  AddObjProperty(&obj, "uid", handle->uid);
  AddObjProperty(&obj, "eventName", handle->eventName);

  return obj;
}
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  return Subscribe(args);
}

JsValue EventsApi::Once(const JsFunctionArguments& args)
{
  return Subscribe(args, true);
}

JsValue EventsApi::Unsubscribe(const JsFunctionArguments& args)
{
  auto obj = args[1];
  auto eventName = std::get<std::string>(
    NativeValueCasts::JsValueToNativeValue(obj.GetProperty("eventName")));
  auto uid = std::get<double>(
    NativeValueCasts::JsValueToNativeValue(obj.GetProperty("uid")));
  EventManager::GetSingleton()->Unsubscribe(uid, eventName);
  return JsValue::Undefined();
}

JsValue EventsApi::SendIpcMessage(const JsFunctionArguments& args)
{
  auto targetSystemName = args[1].ToString();
  auto message = args[2].GetArrayBufferData();
  auto messageLength = args[2].GetArrayBufferLength();

  if (!message || messageLength == 0) {
    throw std::runtime_error(
      "sendIpcMessage expects a valid ArrayBuffer instance");
  }

  IPC::Call(targetSystemName, reinterpret_cast<uint8_t*>(message),
            messageLength);

  return JsValue::Undefined();
}
