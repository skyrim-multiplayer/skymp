#pragma once
#include "EventHandlerMisc.h"
#include "EventHandlerSKSE.h"
#include "EventHandlerScript.h"
#include "EventHandlerStory.h"

struct EventHandle
{
  uintptr_t uid;
  std::string eventName;
};

struct CallbackObject
{
};

struct EventState
{
  const ::Sink* sink;
  std::unordered_set<EventHandle>* subscribers; // <CallbackObject> here
};

class EventManager
{
public:
  [[nodiscard]] static EventManager* GetSingleton()
  {
    static EventManager singleton;
    return &singleton;
  }

  EventHandle Subscribe(std::string eventName) {}
  JsValue Unsubscribe(EventHandle handle);

  void Init()
  {
    // run at main
    // populate this.events from handler.sinks

    /* EventHandlerMisc::GetSingleton()
    EventHandlerSKSE::GetSingleton()
    EventHandlerScript::GetSingleton()
    EventHandlerStory::GetSingleton() */
  }

private:
  EventManager() = default;

  ~EventManager() = default;

  std::unordered_map<const std::string_view, EventState*>* events;
};
