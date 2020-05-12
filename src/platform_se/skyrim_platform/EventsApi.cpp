#include "EventsApi.h"

#include "InvalidArgumentException.h"
#include "MyUpdateTask.h"
#include "NativeObject.h"
#include "NativeValueCasts.h"
#include "NullPointerException.h"
#include <map>
#include <set>
#include <unordered_map>

#include <RE\ConsoleLog.h>

extern ctpl::thread_pool g_pool;
extern TaskQueue g_taskQueue;

struct EventsGlobalState
{
  using Callbacks = std::map<std::string, std::vector<JsValue>>;
  Callbacks callbacks;
  Callbacks callbacksOnce;

  class Handler
  {
  public:
    Handler() = default;

    Handler(const JsValue& handler_)
      : enter(handler_.GetProperty("enter"))
      , leave(handler_.GetProperty("leave"))
    {
    }

    JsValue enter, leave;

    struct PerThread
    {
      JsValue storage, context;
    };

    std::unordered_map<DWORD, PerThread> perThread;
  };

  struct HookInfo
  {
    std::set<DWORD> inProgressThreads;
    std::vector<Handler> handlers;
  };
  HookInfo sendAnimationEvent;
} g;

namespace {
struct SendAnimationEventTag
{
  static constexpr auto name = "sendAnimationEvent";
};
}

namespace {
void CallCalbacks(const char* eventName, const std::vector<JsValue>& arguments,
                  bool isOnce = false)
{
  EventsGlobalState::Callbacks callbacks =
    isOnce ? g.callbacksOnce : g.callbacks;

  if (isOnce)
    g.callbacksOnce[eventName].clear();

  for (auto& f : callbacks[eventName]) {
    f.Call(arguments);
  }
}
}

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  CallCalbacks(eventName, arguments);
  CallCalbacks(eventName, arguments, true);
}

void EventsApi::Clear()
{
  g = {};
}

namespace {
enum class ClearStorage
{
  Yes,
  No
};

void PrepareContext(EventsGlobalState::Handler::PerThread& h,
                    ClearStorage clearStorage)
{
  if (h.context.GetType() != JsValue::Type::Object) {
    h.context = JsValue::Object();
  }

  thread_local auto g_standardMap = JsValue::GlobalObject().GetProperty("Map");
  thread_local auto g_clear =
    g_standardMap.GetProperty("prototype").GetProperty("clear");
  if (h.storage.GetType() != JsValue::Type::Object) {
    h.storage = g_standardMap.Constructor({ g_standardMap });
    h.context.SetProperty("storage", h.storage);
  }

  if (clearStorage == ClearStorage::Yes)
    g_clear.Call({ h.storage });
}
}

void EventsApi::SendAnimationEventEnter(uint32_t selfId,
                                        std::string& animEventName) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is already processing");

      // This should always be done before calling throwing functions
      g.sendAnimationEvent.inProgressThreads.insert(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread[owningThread];
        PrepareContext(perThread, ClearStorage::Yes);

        perThread.context.SetProperty("selfId", (double)selfId);
        perThread.context.SetProperty("animEventName", animEventName);

        h.enter.Call({ JsValue::Undefined(), perThread.context });

        animEventName =
          (std::string)perThread.context.GetProperty("animEventName");
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventEnter)");
      });
    }
  };
  g_pool.push(f).wait();
}

void EventsApi::SendAnimationEventLeave(bool animationSucceeded) noexcept
{
  DWORD owningThread = GetCurrentThreadId();
  auto f = [&](int) {
    try {
      if (!g.sendAnimationEvent.inProgressThreads.count(owningThread))
        throw std::runtime_error("'sendAnimationEvent' is not processing");
      g.sendAnimationEvent.inProgressThreads.erase(owningThread);

      for (auto& h : g.sendAnimationEvent.handlers) {
        auto& perThread = h.perThread.at(owningThread);
        PrepareContext(perThread, ClearStorage::No);

        perThread.context.SetProperty("animationSucceeded",
                                      JsValue::Bool(animationSucceeded));
        h.leave.Call({ JsValue::Undefined(), perThread.context });

        h.perThread.erase(owningThread);
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      g_taskQueue.AddTask([what] {
        throw std::runtime_error(what + " (in SendAnimationEventLeave)");
      });
    }
  };
  g_pool.push(f).wait();
}

namespace {
JsValue CreateHook(EventsGlobalState::HookInfo* hookInfo)
{
  auto hook = JsValue::Object();
  hook.SetProperty(
    "add", JsValue::Function([hookInfo](const JsFunctionArguments& args) {
      auto handlerObj = args[1];
      hookInfo->handlers.push_back(EventsGlobalState::Handler(handlerObj));
      return JsValue::Undefined();
    }));
  return hook;
}
}

JsValue EventsApi::GetHooks()
{
  std::map<std::string, EventsGlobalState::HookInfo*> hooksMap = {
    { "sendAnimationEvent", &g.sendAnimationEvent }
  };

  auto hooks = JsValue::Object();
  for (auto [name, hookInfo] : hooksMap)
    hooks.SetProperty(name, CreateHook(hookInfo));
  return hooks;
}

namespace {
JsValue AddCallback(const JsFunctionArguments& args, bool isOnce = false)
{
  auto eventName = args[1].ToString();
  auto callback = args[2];

  std::set<std::string> events = { "tick", "update" };

  if (events.count(eventName) == 0)
    throw InvalidArgumentException("eventName", eventName);

  isOnce ? g.callbacksOnce[eventName].push_back(callback)
         : g.callbacks[eventName].push_back(callback);
  return JsValue::Undefined();
}
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  return AddCallback(args);
}

JsValue EventsApi::Once(const JsFunctionArguments& args)
{
  return AddCallback(args, true);
}
