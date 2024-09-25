#include "EventsApi.h"
#include "EventManager.h"
#include "ExceptionPrinter.h"
#include "Handler.h"
#include "Hook.h"
#include "HookPattern.h"
#include "IPC.h"
#include "InvalidArgumentException.h"
#include "JsUtils.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include "SkyrimPlatform.h"
#include "ThreadPoolWrapper.h"

struct EventsGlobalState
{
  EventsGlobalState()
  {
    sendAnimationEvent.reset(
      new Hook("sendAnimationEvent", "animEventName", "animationSucceeded"));
    sendPapyrusEvent.reset(
      new Hook("sendPapyrusEvent", "papyrusEventName", std::nullopt));
  }

  using Callbacks =
    std::map<std::string,
             std::vector<std::shared_ptr<Napi::Reference<Napi::Function>>>>;
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
  for (const auto& [uid, cb] : *cbObjMap) {
    callbacksToCall.push_back(cb);
    if (cb.runOnce) {
      callbacksToUnsubscribe.push_back(uid);
    }
  }

  // 2. Make sure that "runOnce" callbacks will never be called again
  for (auto uid : callbacksToUnsubscribe) {
    manager->Unsubscribe(uid, eventName);
  }

  // 3. Finally, call the callbacks
  for (auto& cb : callbacksToCall) {
    try {
      Napi::Function callback = cb.callback->Value().As<Napi::Function>();
      callback.Call(env.Undefined(), arguments);
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
  hook.Set(
    "add",
    Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo& info) {
      auto handlerObj = NapiHelper::ExtractObject(info[0], "handlerObj");

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

      Handler handler(handlerObj, minSelfId, maxSelfId, pattern);
      uint32_t id = hookInfo->AddHandler(handler);

      return Napi::Number::New(env, id);
    }));

  hook.Set(
    "remove",
    Napi::Function::New(env, [hookInfo](const Napi::CallbackInfo& info) {
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
Napi::Value Subscribe(const Napi::CallbackInfo& info, bool runOnce = false)
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
  auto jEventName =
    NapiHelper::ExtractString(obj.Get("eventName"), "obj.eventName");
  auto jUid = NapiHelper::ExtractUInt32(obj.Get("uid"), "obj.uid");
  auto eventName =
    std::get<std::string>(NativeValueCasts::JsValueToNativeValue(jEventName));
  auto uid = std::get<double>(NativeValueCasts::JsValueToNativeValue(jUid));
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
