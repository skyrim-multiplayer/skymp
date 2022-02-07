#pragma once
#include "../InvalidArgumentException.h"
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

struct SinkObject
{
  SinkObject(const ::Sink* _sink, std::vector<std::string_view>* _linkedEvents)
    : sink(_sink)
    , linkedEvents(_linkedEvents)
  {
  }
  const ::Sink* sink;
  const std::vector<std::string_view>* linkedEvents;
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
  EventState(const SinkObject* _sink)
    : sinkObj(_sink)
  {
  }

  const SinkObject* sinkObj;
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
    if (event->sinkObj && !event->sinkObj->sink->IsActive())
      event->sinkObj->sink->Activate();

    // use callback pointer as unique id for that callback
    auto uid = reinterpret_cast<uintptr_t>(&callback);
    event->callbacks->emplace(uid, new CallbackObject(&callback, runOnce));

    return new EventHandle(uid, eventName);
  }

  void Unsubscribe(uintptr_t uid, std::string eventName)
  {
    // check for correct event
    auto event = (*events)[eventName];
    if (!event)
      return;

    // check if callback with provided uid exists
    if (!event->callbacks->contains(uid))
      return;

    event->callbacks->erase(uid);

    // now we need to see if we can deactivate event sink

    // ignore this for custom events
    if (event->sinkObj) {
      // check if there are no other callbacks for this event
      if (event->callbacks->empty()) {
        // check if there are any linked events for this sink
        if (event->sinkObj->linkedEvents->empty()) {
          event->sinkObj->sink->Deactivate();
        } else {
          // check if there are any callbacks for linked events
          auto sinkIsBusy = false;
          for (const auto& eventName : *event->sinkObj->linkedEvents) {
            if (!(*events)[eventName]->callbacks->empty()) {
              sinkIsBusy = true;
              break;
            }
          }

          if (!sinkIsBusy) {
            event->sinkObj->sink->Deactivate();
          }
        }
      }
    }
  }

  std::unordered_map<uintptr_t, CallbackObject*>* GetCallbackObjects(
    const char* eventName)
  {
    auto event = (*events)[eventName];

    if (!event)
      return nullptr;

    return event->callbacks;
  }

  /**
   * @brief Fetches collections of supported events from corresponding event
   * handlers. Populates global event map with event names, corresponding Sink
   * class instance and empty callback collection, which is being filled when
   * JS plugin subscribes to an event.
   */
  void Init()
  {
    if (const auto misc = EventHandlerMisc::GetSingleton()->GetSinks()) {
      ProcessSinks(misc);
    }

    if (const auto skse = EventHandlerSKSE::GetSingleton()->GetSinks()) {
      ProcessSinks(skse);
    }

    if (const auto script = EventHandlerScript::GetSingleton()->GetSinks()) {
      ProcessSinks(script);
    }

    if (const auto story = EventHandlerStory::GetSingleton()->GetSinks()) {
      ProcessSinks(story);
    }

    ProcessCustomEvents();
  }

private:
  EventManager() = default;
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  void ProcessSinks(SinkSet map)
  {
    for (const auto& item : *map) {
      for (const auto& eventName : *item.first) {
        std::vector<std::string_view> linkedEvents;
        std::copy_if(item.first->begin(), item.first->end(),
                     std::back_inserter(linkedEvents),
                     [&](const char* const s) { return s != eventName; });

        auto sinkObj = new SinkObject(item.second, &linkedEvents);
        events->emplace(eventName, new EventState(sinkObj));
      }
    }
  }

  /**
   * @brief If we create our own events that fire from within the code
   * we register them with nullptr SinkObj here
   */
  void ProcessCustomEvents()
  {
    events->emplace("update", new EventState(nullptr));
    events->emplace("tick", new EventState(nullptr));
    events->emplace("browserMessage", new EventState(nullptr));
    events->emplace("consoleMessage", new EventState(nullptr));
    events->emplace("ipcMessage", new EventState(nullptr));
  }

  robin_hood::unordered_map<std::string_view, EventState*>* events;
};
