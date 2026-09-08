#include "EventsApi.h"
#include "EventManager.h"
#include "ExceptionPrinter.h"
#include "Hook.h"
#include "HookPattern.h"
#include "HooksStorage.h"
#include "IPC.h"
#include "InvalidArgumentException.h"
#include "JsUtils.h"
#include "NullPointerException.h"
#include "ScopedTask.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"

class EventsGlobalState
{
public:
  EventsGlobalState()
  {
    sendAnimationEvent.reset(
      new Hook("sendAnimationEvent", "animEventName", "animationSucceeded"));
    sendPapyrusEvent.reset(
      new Hook("sendPapyrusEvent", "papyrusEventName", std::nullopt));
  }

  std::shared_ptr<Hook> sendAnimationEvent;
  std::shared_ptr<Hook> sendPapyrusEvent;
} g;

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<Napi::Value>& arguments)
{
  auto manager = EventManager::GetSingleton();

  const CallbackObjMap& cbObjMap = manager->GetCallbackObjMap(eventName);

  // TODO: use container with pre-allocated space
  // Global vars aren't helpful, because SendEvent can be made recursive in the
  // future.
  std::vector<uintptr_t> callbacksToUnsubscribe;
  std::vector<CallbackObject> callbacksToCall;

  // 1. Collect all callbacks and remember "runOnce" callbacks
  for (const auto& [uid, cb] : cbObjMap) {
    callbacksToCall.push_back(cb);
    if (cb.runOnce) {
      callbacksToUnsubscribe.push_back(uid);
    }
  }

  // 2. Make sure that "runOnce" callbacks will never be called again
  for (uintptr_t uid : callbacksToUnsubscribe) {
    manager->Unsubscribe(uid, eventName);
  }

  // 3. Finally, call the callbacks
  for (CallbackObject& cb : callbacksToCall) {
    try {
      Napi::Function callback = cb.callback->Value().As<Napi::Function>();
      callback.Call(arguments);
    } catch (const std::exception& e) {
      const char* method = cb.runOnce ? "once" : "on";
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

  // hooks.sendAnimationEvent.add(sourceCode, [minSelfId], [maxSelfId],
  // [pattern])
  // sourceCode: string containing JS with enter(ctx) and/or leave(ctx)
  hook.Set(
    "add",
    Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo& info) {
      auto source = NapiHelper::ExtractString(info[0], "source");

      std::optional<double> minSelfId;
      if (info[1].IsNumber()) {
        minSelfId = NapiHelper::ExtractDouble(info[1], "minSelfId");
      }

      std::optional<double> maxSelfId;
      if (info[2].IsNumber()) {
        maxSelfId = NapiHelper::ExtractDouble(info[2], "maxSelfId");
      }

      std::optional<HookPattern> pattern;
      if (info[3].IsString()) {
        auto s = NapiHelper::ExtractString(info[3], "pattern");
        pattern = HookPattern::Parse(s);
      }

      uint32_t id =
        hookInfo->AddScript(source, minSelfId, maxSelfId, std::move(pattern));

      return Napi::Number::New(info.Env(), id);
    }));

  hook.Set(
    "remove",
    Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo& info) {
      uint32_t toRemove = NapiHelper::ExtractUInt32(info[0], "toRemove");
      hookInfo->RemoveScript(toRemove);
      return info.Env().Undefined();
    }));
  return hook;
}
}

Napi::Value EventsApi::GetHooks(Napi::Env env)
{
  auto res = Napi::Object::New(env);
  for (auto& hook : { g.sendAnimationEvent, g.sendPapyrusEvent }) {
    res.Set(hook->GetName(), CreateHookApi(env, hook));
  }
  return res;
}

Napi::Value EventsApi::GetHooksStorage(Napi::Env env)
{
  auto obj = Napi::Object::New(env);

  obj.Set("get", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
    auto key = NapiHelper::ExtractString(info[0], "key");
    auto val = HooksStorage::GetSingleton().Get(key);

    Napi::Env env = info.Env();
    if (std::holds_alternative<std::monostate>(val)) {
      return env.Undefined();
    }
    if (std::holds_alternative<bool>(val)) {
      return static_cast<Napi::Value>(
        Napi::Boolean::New(env, std::get<bool>(val)));
    }
    if (std::holds_alternative<double>(val)) {
      return static_cast<Napi::Value>(
        Napi::Number::New(env, std::get<double>(val)));
    }
    if (std::holds_alternative<std::string>(val)) {
      return static_cast<Napi::Value>(
        Napi::String::New(env, std::get<std::string>(val)));
    }
    return env.Undefined();
  }));

  obj.Set("set", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
    auto key = NapiHelper::ExtractString(info[0], "key");
    Napi::Value val = info[1];

    if (val.IsBoolean()) {
      HooksStorage::GetSingleton().Set(key, val.As<Napi::Boolean>().Value());
    } else if (val.IsNumber()) {
      HooksStorage::GetSingleton().Set(key,
                                       val.As<Napi::Number>().DoubleValue());
    } else if (val.IsString()) {
      HooksStorage::GetSingleton().Set(
        key, val.As<Napi::String>().Utf8Value());
    } else {
      HooksStorage::GetSingleton().Set(key, std::monostate{});
    }

    return info.Env().Undefined();
  }));

  obj.Set("has", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
    auto key = NapiHelper::ExtractString(info[0], "key");
    return Napi::Boolean::New(info.Env(),
                              HooksStorage::GetSingleton().Has(key));
  }));

  obj.Set(
    "erase", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
      auto key = NapiHelper::ExtractString(info[0], "key");
      HooksStorage::GetSingleton().Erase(key);
      return info.Env().Undefined();
    }));

  obj.Set(
    "clear", Napi::Function::New(env, [](const Napi::CallbackInfo& info) {
      HooksStorage::GetSingleton().Clear();
      return info.Env().Undefined();
    }));

  return obj;
}

namespace {
Napi::Value Subscribe(const Napi::CallbackInfo& info, bool runOnce = false)
{
  auto eventName = NapiHelper::ExtractString(info[0], "eventName");
  auto callback = NapiHelper::ExtractFunction(info[1], "callback");

  std::shared_ptr<Napi::Reference<Napi::Function>> callbackRef;
  callbackRef.reset(
    new Napi::Reference<Napi::Function>(Napi::Persistent(callback)));

  auto handle =
    EventManager::GetSingleton()->Subscribe(eventName, callbackRef, runOnce);

  auto obj = Napi::Object::New(info.Env());
  AddObjProperty(&obj, "uid", handle->uid);
  AddObjProperty(&obj, "eventName", handle->eventName);

  return obj;
}
}

Napi::Value EventsApi::On(const Napi::CallbackInfo& info)
{
  return Subscribe(info);
}

Napi::Value EventsApi::Once(const Napi::CallbackInfo& info)
{
  return Subscribe(info, true);
}

Napi::Value EventsApi::Unsubscribe(const Napi::CallbackInfo& info)
{
  auto obj = NapiHelper::ExtractObject(info[0], "obj");
  auto eventName =
    NapiHelper::ExtractString(obj.Get("eventName"), "obj.eventName");
  auto uid = NapiHelper::ExtractUInt32(obj.Get("uid"), "obj.uid");
  EventManager::GetSingleton()->Unsubscribe(uid, eventName);
  return info.Env().Undefined();
}

Napi::Value EventsApi::SendIpcMessage(const Napi::CallbackInfo& info)
{
  Napi::Env env = info.Env();

  std::string targetSystemName =
    NapiHelper::ExtractString(info[0], "targetSystemName");
  Napi::ArrayBuffer messageBuffer =
    NapiHelper::ExtractArrayBuffer(info[1], "message");

  void* message = messageBuffer.Data();
  size_t messageLength = messageBuffer.ByteLength();

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

  return env.Undefined();
}
