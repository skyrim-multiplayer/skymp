#pragma once
#include "EventHandlerMisc.h"
#include "EventHandlerSKSE.h"
#include "EventHandlerScript.h"
#include "EventHandlerStory.h"
#include "InvalidArgumentException.h"

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
  CallbackObject(JsValue* _callback, bool _runOnce)
    : callback(_callback)
    , runOnce(_runOnce)
  {
  }

  JsValue* callback;
  bool runOnce;
};

struct EventState
{
  EventState(const ::Sink* _sink)
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

  EventHandle* Subscribe(std::string eventName, JsValue callback, bool runOnce)
  {
    // check if event is supported
    auto event = (*events)[eventName];
    if (!event) {
      logger::critical("Subscription to event failed. "
                       "{} is not a valid argument for eventName",
                       eventName);
      throw InvalidArgumentException("eventName", eventName);
      return new EventHandle(0, "");
    }

    // if sink for that event is not active activate it, duh
    if (!event->sink->IsActive())
      event->sink->Activate();

    // use callback pointer as unique id for that callback
    auto uid = reinterpret_cast<uintptr_t>(&callback);
    event->callbacks->emplace(uid, new CallbackObject(&callback, runOnce));

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
      event->sink->Deactivate();

    return true;
  }

  /**
   * @brief Fetches collections of supported events from corresponding event
   * handlers. Populates global event map with event names, corresponding Sink
   * class instance and empty callback collection, which is being filled when
   * JS plugin subscribes to an event.
   */
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
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  void ProcessMap(EventMap map)
  {
    for (const auto& item : *map) {
      if (item.first->size() == 1) {
        events->emplace((*item.first)[0], new EventState(item.second));
      } else {
        for (const auto& eventName : *item.first) {
          events->emplace(eventName, new EventState(item.second));
        }
      }
    }
  }

  std::unordered_map<const std::string_view, EventState*>* events;
};
