#include "Hook.h"
#include "Handler.h"

Hook::Hook(std::string hookName_, std::string eventNameVariableName_,
       std::optional<std::string> succeededVariableName_)
    : hookName(hookName_)
    , eventNameVariableName(eventNameVariableName_)
    , succeededVariableName(succeededVariableName_)
{
}

uint32_t Hook::AddHandler(const Handler& handler)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to add hook inside hook context");
    }
    handlers.emplace(hCounter, handler);
    return hCounter++;
  }

  void Hook::RemoveHandler(const uint32_t& id)
  {
    if (addRemoveBlocker) {
      throw std::runtime_error("Trying to remove hook inside hook context");
    }
    handlers.erase(id);
  }

  std::string Hook::GetName() const { return hookName; }

  void Hook::Enter(uint32_t selfId, std::string& eventName)
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

  void Hook::Leave(bool succeeded)
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

  void Hook::HandleEnter(DWORD owningThread, uint32_t selfId, std::string& eventName, const Napi::Env& env)
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

  void Hook::PrepareContext(Handler::PerThread& h, const Napi::Env& env)
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

  void Hook::ClearContextStorage(Handler::PerThread& h, Napi::Env env)
  {
    Napi::Object global = env.Global();
    Napi::Object standardMap = global.Get("Map").As<Napi::Object>();
    Napi::Function clear = standardMap.Get("prototype").As<Napi::Object>().Get("clear").As<Napi::Function>();

    clear.Call(h.storage, {});
  }

  void Hook::HandleLeave(DWORD owningThread, bool succeeded, Napi::Env env) {
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
