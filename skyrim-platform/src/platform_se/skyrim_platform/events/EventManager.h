#pragma once
#include "EventHandlerMisc.h"
#include "EventHandlerSKSE.h"
#include "EventHandlerScript.h"
#include "EventHandlerStory.h"

struct EventHandle
{
  EventHandle(uintptr_t _uid, std::string_view _eventName)
    : uid(_uid)
    , eventName(_eventName)
  {
  }

  uintptr_t uid;
  std::string_view eventName;
};

struct CallbackObject
{
  CallbackObject(JsValue* _callback, bool _onceFlag)
    : callback(_callback)
    , onceFlag(_onceFlag)
  {
  }

  JsValue* callback;
  bool onceFlag;
};

struct EventState
{
  EventState(::Sink _sink)
    : sink(_sink)
  {
  }

  const ::Sink* sink;
  std::unordered_map<uintptr_t, CallbackObject*>* callbacks;
};

class EventManager
{
public:
  [[nodiscard]] static EventManager* GetSingleton()
  {
    static EventManager singleton;
    return &singleton;
  }

  EventHandle* Subscribe(std::string eventName, JsValue callback,
                         bool onceFlag = false)
  {
    auto event = (*events)[eventName];
    if (!event)
      return nullptr;

    if (!event->sink->IsActive())
      event->sink->Add();

    auto uid = reinterpret_cast<uintptr_t>(&callback);
    event->callbacks->emplace(uid, new CallbackObject(&callback, onceFlag));

    return new EventHandle(uid, eventName);
  }

  bool Unsubscribe(EventHandle* handle)
  {
    auto event = (*events)[handle->eventName];
    if (!event)
      return false;

    if (!event->callbacks->contains(handle->uid))
      return false;

    event->callbacks->erase(handle->uid);

    if (event->callbacks->size() == 0)
      event->sink->Remove();

    return true;
  }

  void Init()
  {
    if (const auto misc = EventHandlerMisc::GetSingleton()->FetchEvents()) {
      ProcessMap(misc);
    }

    if (const auto skse = EventHandlerSKSE::GetSingleton()->FetchEvents()) {
      ProcessMap(skse);
    }

    if (const auto script =
          EventHandlerScript::GetSingleton()->FetchEvents()) {
      ProcessMap(script);
    }

    if (const auto story = EventHandlerStory::GetSingleton()->FetchEvents()) {
      ProcessMap(story);
    }
  }

private:
  EventManager() = default;
  ~EventManager() = default;

  void ProcessMap(EventMap map)
  {
    for (const auto& item : *map) {
      if (item.first->size() == 1) {
        events->emplace((*item.first)[0], new EventState(item.second));
      } else {
        for (const auto& eventName : *item.first) {
          events->emplace(eventName, new EventState(item.second););
        }
      }
    }
  }

  std::unordered_map<const std::string_view, EventState*>* events;
};
