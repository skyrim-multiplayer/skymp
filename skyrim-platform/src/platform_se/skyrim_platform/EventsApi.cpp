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

  Handler(const Napi::Value& handler_, std::optional<double> minSelfId_,
          std::optional<double> maxSelfId_, std::optional<Pattern> pattern_)
    : enter(Napi::Persistent(handler_.Get("enter").As<Napi::Function>()))
    , leave(Napi::Persistent(handler_.Get("leave").As<Napi::Function>()))
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
    std::shared_ptr<Napi::Reference<Napi::Object>> storage, context;
    bool matchesCondition = false;
  };
  std::unordered_map<DWORD, PerThread> perThread;

  // Shared between threads
  const Napi::Reference<Napi::Function> enter, leave;
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
        [this, owningThread, selfId, eventName](Napi::Env env) {
          std::string s = eventName;
          HandleEnter(owningThread, selfId, s, env);
        });
    }

    auto f = [&](Napi::Env env) {
      try {
        if (inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is already processing");
        inProgressThreads.insert(owningThread);
        HandleEnter(owningThread, selfId, eventName, env);
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

    auto f = [&](Napi::Env env) {
      try {
        if (!inProgressThreads.count(owningThread))
          throw std::runtime_error("'" + hookName + "' is not processing");
        inProgressThreads.erase(owningThread);
        HandleLeave(owningThread, succeeded, env);

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
  void HandleEnter(DWORD owningThread, uint32_t selfId, std::string& eventName, const Napi::Env& env)
  {
    for (auto& hp : handlers) {
      Handler* h = &hp.second;
      auto& perThread = h->perThread[owningThread];

      perThread.matchesCondition = h->Matches(selfId, eventName);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread, env);
      ClearContextStorage(perThread, env);

      perThread.context->Value().As<Napi::Object>().Set("selfId", Napi::Number::New(env, static_cast<double>(selfId)));
      perThread.context->Value().As<Napi::Object>().Set(eventNameVariableName, Napi::String::New(env, eventName));
      h->enter.As<Napi::Function>().Call({ env.Undefined(), perThread.context });

      // Retrieve the updated eventName from the context
      Napi::Value updatedEventName = perThread.context.As<Napi::Object>().Get(eventNameVariableName);
      eventName = updatedEventName.As<Napi::String>().Utf8Value();
    }
  }

  void PrepareContext(Handler::PerThread& h, const Napi::Env& env)
  {
    if (!h.context) {
      auto object = Napi::Object::New(env);
      h.context.reset(new Napi::Reference<Napi::Object>(
        Napi::Persistent<Napi::Object>(object)));
    }

    Napi::Value standardMap = env.Global().Get("Map");

    if (!h.storage.IsObject()) {
      Napi::Object mapInstance = standardMap.As<Napi::Function>().New({});
      h.storage = mapInstance;
      h.context.As<Napi::Object>().Set("storage", h.storage);
    }
  }

  void ClearContextStorage(Handler::PerThread& h, Napi::Env env)
  {
    Napi::Object global = env.Global();
    Napi::Object standardMap = global.Get("Map").As<Napi::Object>();
    Napi::Function clear = standardMap.Get("prototype").As<Napi::Object>().Get("clear").As<Napi::Function>();

    clear.Call(h.storage, {});
  }

  void HandleLeave(DWORD owningThread, bool succeeded, Napi::Env env)
  {
    for (auto& hp : handlers) {
      auto* h = &hp.second;
      auto& perThread = h->perThread.at(owningThread);
      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread, env);

      if (succeededVariableName.has_value()) {
        perThread.context.Set(succeededVariableName.value(),
                                      Napi::Value::Bool(succeeded));
      }
      h->leave.Call({ Napi::Value::Undefined(), perThread.context });
      h->perThread.erase(owningThread);
    }
  }

  void HandleLeave(DWORD owningThread, bool succeeded, Napi::Env env) {
    for (auto& hp : handlers) {
      Handler* h = &hp.second;
      auto& perThread = h->perThread.at(owningThread);

      if (!perThread.matchesCondition) {
        continue;
      }

      PrepareContext(perThread, env);

      if (succeededVariableName.has_value()) {
        perThread.context->Value().As<Napi::Object>().Set(
          succeededVariableName.value(),
          Napi::Boolean::New(env, succeeded)
        );
      }

      h->leave->Value().As<Napi::Function>().Call({ env.Undefined(), perThread.context });
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

  using Callbacks = std::map<std::string, std::vector<std::shared_ptr<Napi::Reference<Napi::Function>>>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;
  std::shared_ptr<Hook> sendAnimationEvent;
  std::shared_ptr<Hook> sendPapyrusEvent;
} g;

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<Napi::Value>& arguments)
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
      Napi::Function callback = callbackInfo.callback->Value().As<Napi::Function>();
      callback.Call(env.Undefined(), arguments);
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
Napi::Value CreateHookApi(Napi::Env env, std::shared_ptr<Hook> hookInfo)
{
  auto hook = Napi::Object::New(env);
  hook.Set(
    "add", Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo &info) {
      auto handlerObj = NapiHelper::ExtractObject(info[0], "handlerObj");

      std::optional<double> minSelfId;
      if (info[1].IsNumber()) {
        minSelfId = NapiHelper::ExtractDouble(info[1], "minSelfId");
      }

      std::optional<double> maxSelfId;
      if (info[2].IsNumber()) {
        maxSelfId = NapiHelper::ExtractDouble(info[2], "maxSelfId");
      }

      std::optional<Pattern> pattern;
      if (info[3].IsString()) {
        auto s = NapiHelper::ExtractString(info[3], "pattern");
        pattern = Pattern::Parse(s);
      }

      Handler handler(handlerObj, minSelfId, maxSelfId, pattern);
      uint32_t id = hookInfo->AddHandler(handler);

      return Napi::Number::New(env, id);
    }));

  hook.Set(
    "remove", Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo &info) {
      uint32_t toRemove = NapiHelper::ExtractUInt32(info[0], "toRemove");
      hookInfo->RemoveHandler(toRemove);
      return info.Env().Undefined();
    }));
  return hook;
}
}

Napi::Value EventsApi::GetHooks(Napi::Env env)
{
  auto res = Napi::Object::New(env);
  for (auto& hook : { g.sendAnimationEvent, g.sendPapyrusEvent }) {
    res.Set(hook->GetName(), CreateHookApi(hook));
  }
  return res;
}

namespace {
Napi::Value Subscribe(const Napi::CallbackInfo &info, bool runOnce = false)
{
  auto eventName = NapiHelper::ExtractString(info[0], "eventName");
  auto callback = NapiHelper::ExtractFunction(info[1], "callback");

  auto handle =
    EventManager::GetSingleton()->Subscribe(eventName, callback, runOnce);

  auto obj = Napi::Object::New(env);
  AddObjProperty(&obj, "uid", handle->uid);
  AddObjProperty(&obj, "eventName", handle->eventName);

  return obj;
}
}

Napi::Value EventsApi::On(const Napi::CallbackInfo &info)
{
  return Subscribe(args);
}

Napi::Value EventsApi::Once(const Napi::CallbackInfo &info)
{
  return Subscribe(args, true);
}

Napi::Value EventsApi::Unsubscribe(const Napi::CallbackInfo &info)
{
  auto obj = NapiHelper::ExtractObject(info[0], "obj");
  auto jEventName = NapiHelper::ExtractString(obj.Get("eventName"), "obj.eventName");
  auto jUid = NapiHelper::ExtractUInt32(obj.Get("uid"), "obj.uid");
  auto eventName = std::get<std::string>(
    NativeValueCasts::Napi::ValueToNativeValue(jEventName));
  auto uid = std::get<double>(
    NativeValueCasts::Napi::ValueToNativeValue(jUid));
  EventManager::GetSingleton()->Unsubscribe(uid, eventName);
  return info.Env().Undefined();
}

Napi::Value EventsApi::SendIpcMessage(const Napi::CallbackInfo &info)
{
  auto targetSystemName = args[1].ToString();
  auto message = args[2].GetArrayBufferData();
  auto messageLength = args[2].GetArrayBufferLength();

  if (!message) {
    throw std::runtime_error(
      "sendIpcMessage expects a valid ArrayBuffer instance");
  }

  if (messageLength == 0) {
    throw std::runtime_error(
      "sendIpcMessage expects an ArrayBuffer of length > 0");
  }

  IPC::Call(targetSystemName, reinterpret_cast<uint8_t*>(message),
            messageLength);

  return Napi::Value::Undefined();
}

// TODO: review
Napi::Value EventsApi::SendIpcMessage(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  // Ensure we have at least two arguments: targetSystemName and the ArrayBuffer
  if (info.Length() < 2 || !info[0].IsString() || !info[1].IsArrayBuffer()) {
    Napi::TypeError::New(env, "Expected a string and an ArrayBuffer as arguments").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Extract the target system name (a string)
  std::string targetSystemName = info[0].As<Napi::String>().Utf8Value();

  // Extract the message (an ArrayBuffer)
  Napi::ArrayBuffer messageBuffer = info[1].As<Napi::ArrayBuffer>();
  void* message = messageBuffer.Data();
  size_t messageLength = messageBuffer.ByteLength();

  // Check if the message is valid
  if (!message) {
    Napi::Error::New(env, "sendIpcMessage expects a valid ArrayBuffer instance").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Check if the message length is valid
  if (messageLength == 0) {
    Napi::Error::New(env, "sendIpcMessage expects an ArrayBuffer of length > 0").ThrowAsJavaScriptException();
    return env.Null();
  }

  // Call the IPC function, passing the message data and length
  IPC::Call(targetSystemName, reinterpret_cast<uint8_t*>(message), messageLength);

  // Return undefined as the function does not have a return value
  return env.Undefined();
}
