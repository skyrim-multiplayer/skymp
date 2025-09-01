#include "Hook.h"
#include "Handler.h"
#include "ScopedTask.h"
#include "SkyrimPlatform.h"
#include <stack>

namespace {
// Hardcoded to serve sendAnimationEvent hook, will be broken if we add any
// However, there were no new real hooks in SkyrimPlatform API since 2020

std::mutex g_invocationsMutex;
std::map<DWORD, std::stack<HookInvocationInfo>> g_invocations;
}

Hook::Hook(std::string hookName_, std::string eventNameVariableName_,
           std::optional<std::string> succeededVariableName_)
  : hookName(hookName_)
  , eventNameVariableName(eventNameVariableName_)
  , succeededVariableName(succeededVariableName_)
{
}

uint32_t Hook::AddHandler(const std::shared_ptr<Handler>& handler)
{
  if (addRemoveBlocker) {
    throw std::runtime_error("Trying to add hook inside hook context");
  }
  handlers.emplace(nextHandlerId, handler);

  const uint32_t handlerId = nextHandlerId++;

  return handlerId;
}

void Hook::RemoveHandler(const uint32_t& id)
{
  if (addRemoveBlocker) {
    throw std::runtime_error("Trying to remove hook inside hook context");
  }

  handlers.erase(id);
}

std::string Hook::GetName() const
{
  return hookName;
}

void Hook::Enter(uint32_t selfId, std::string& eventName)
{
  if (hookName == "sendPapyrusEvent") {
    return SendPapyrusEventHandleEnter(selfId, eventName);
  }

  addRemoveBlocker++;
  Viet::ScopedTask<Hook> t([](Hook& hook) { hook.addRemoveBlocker--; }, *this);

  HookInvocationInfo* invocationInfo = nullptr;
  const DWORD threadId = GetCurrentThreadId();
  {
    std::lock_guard l(g_invocationsMutex);
    auto& invocationsStack = g_invocations[threadId];
    invocationsStack.push(HookInvocationInfo());
    invocationInfo = &invocationsStack.top();
  }

  auto handle = [&](Napi::Env env) {
    try {
      HandleEnter(*invocationInfo, selfId, eventName, env);
    } catch (std::exception& e) {
      auto err = std::string(e.what()) + " (while performing enter on '" +
        hookName + "')";
      SkyrimPlatform::GetSingleton()->AddUpdateTask(
        [err](Napi::Env) { throw std::runtime_error(err); });
    }
  };

  JsEngine::GetSingleton()->AcquireEnvAndCall(handle, "HookEnter");
}

void Hook::Leave(bool succeeded)
{
  if (hookName == "sendPapyrusEvent") {
    return; // No-op for sendPapyrusEvent pseudo-hook
  }

  addRemoveBlocker++;
  Viet::ScopedTask<Hook> t([](Hook& hook) { hook.addRemoveBlocker--; }, *this);

  HookInvocationInfo* invocationInfo = nullptr;
  const DWORD threadId = GetCurrentThreadId();
  {
    std::lock_guard l(g_invocationsMutex);
    auto& invocationsStack = g_invocations[threadId];

    if (invocationsStack.empty()) {
      return;
    }

    invocationInfo = &invocationsStack.top();
  }

  auto handle = [&](Napi::Env env) {
    try {
      HandleLeave(*invocationInfo, succeeded, env);
    } catch (std::exception& e) {
      auto err = std::string(e.what()) + " (while performing leave on '" +
        hookName + "')";
      SkyrimPlatform::GetSingleton()->AddUpdateTask(
        [err](Napi::Env) { throw std::runtime_error(err); });
    }
  };

  JsEngine::GetSingleton()->AcquireEnvAndCall(handle, "HookLeave");

  {
    std::lock_guard l(g_invocationsMutex);
    auto& invocationsStack = g_invocations[threadId];

    if (invocationsStack.empty()) {
      return;
    }

    invocationsStack.pop();
  }
}

void Hook::SendPapyrusEventHandleEnter(uint32_t selfId, std::string& eventName)
{
  /*bool anyMatch = false;
  for (auto& hp : handlers) {
    auto* h = hp.second.get();
    if (h->Matches(selfId, eventName)) {
      anyMatch = true;
      break;
    }
  }
  if (!anyMatch) {
    return;
  }

  return SkyrimPlatform::GetSingleton()->AddUpdateTask(
    [this, selfId, eventName](Napi::Env env) {
      std::string s = eventName;
      HookInvocationInfo tmpInvocationInfo;
      HandleEnter(tmpInvocationInfo, selfId, s, env);
    });*/
}

void Hook::HandleEnter(HookInvocationInfo& hookInvocationInfo, uint32_t selfId,
                       std::string& eventName, const Napi::Env& env)
{
  for (auto& hp : handlers) {
    const auto hId = hp.first;
    Handler* h = hp.second.get();

    auto& handlerInvocationInfo = hookInvocationInfo.handlersInvocationInfo[h];

    handlerInvocationInfo.matchesCondition = h->Matches(selfId, eventName);
    if (!handlerInvocationInfo.matchesCondition) {
      continue;
    }

    PrepareContext(handlerInvocationInfo, env);
    ClearContextStorage(handlerInvocationInfo, env);

    handlerInvocationInfo.context->Value().As<Napi::Object>().Set(
      "selfId", Napi::Number::New(env, static_cast<double>(selfId)));
    handlerInvocationInfo.context->Value().As<Napi::Object>().Set(
      eventNameVariableName, Napi::String::New(env, eventName));
    h->enter.Value().As<Napi::Function>().Call(
      env.Undefined(), { handlerInvocationInfo.context->Value() });

    // Retrieve the updated eventName from the context
    Napi::Value updatedEventName =
      handlerInvocationInfo.context->Value().As<Napi::Object>().Get(
        eventNameVariableName);

    eventName = updatedEventName.As<Napi::String>().Utf8Value();
  }
}

void Hook::HandleLeave(HookInvocationInfo& hookInvocationInfo, bool succeeded,
                       Napi::Env env)
{
  for (auto& hp : handlers) {
    Handler* h = hp.second.get();

    auto& handlerInvocationInfo = hookInvocationInfo.handlersInvocationInfo[h];

    if (!handlerInvocationInfo.matchesCondition) {
      continue;
    }

    PrepareContext(handlerInvocationInfo, env);

    if (succeededVariableName.has_value()) {
      handlerInvocationInfo.context->Value().As<Napi::Object>().Set(
        succeededVariableName.value(), Napi::Boolean::New(env, succeeded));
    }

    h->leave.Value().As<Napi::Function>().Call(
      env.Undefined(), { handlerInvocationInfo.context->Value() });
  }
}

void Hook::PrepareContext(HandlerInvocationInfo& handlerInvocationInfo,
                          const Napi::Env& env)
{
  if (!handlerInvocationInfo.context) {
    auto object = Napi::Object::New(env);
    handlerInvocationInfo.context.reset(new Napi::Reference<Napi::Object>(
      Napi::Persistent<Napi::Object>(object)));
  }

  Napi::Value standardMap = env.Global().Get("Map");

  if (!handlerInvocationInfo.storage) {
    Napi::Object mapInstance = standardMap.As<Napi::Function>().New({});
    handlerInvocationInfo.storage.reset(
      new Napi::Reference<Napi::Object>(Napi::Persistent(mapInstance)));
    handlerInvocationInfo.context->Value().As<Napi::Object>().Set(
      "storage", handlerInvocationInfo.storage->Value());
  }
}

void Hook::ClearContextStorage(HandlerInvocationInfo& handlerInvocationInfo,
                               Napi::Env env)
{
  if (!handlerInvocationInfo.storage) {
    return;
  }

  Napi::Object global = env.Global();
  Napi::Object standardMap = global.Get("Map").As<Napi::Object>();
  Napi::Function clear = standardMap.Get("prototype")
                           .As<Napi::Object>()
                           .Get("clear")
                           .As<Napi::Function>();

  clear.Call(handlerInvocationInfo.storage->Value(), {});
}
