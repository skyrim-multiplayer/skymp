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

using CallbackObjMap = robin_hood::unordered_map<uintptr_t, CallbackObject*>*;

struct EventState
{
  EventState(const SinkObject* _sink)
    : sinkObj(_sink)
  {
  }

  const SinkObject* sinkObj;
  CallbackObjMap callbacks;
};

using EventMap = robin_hood::unordered_map<std::string_view, EventState*>*;

class EventManager
{
public:
  [[nodiscard]] static EventManager* GetSingleton()
  {
    static EventManager singleton;
    return &singleton;
  }

  EventHandle* Subscribe(std::string eventName, JsValue callback,
                         bool runOnce);

  void Unsubscribe(uintptr_t uid, std::string eventName);

  CallbackObjMap GetCallbackObjMap(const char* eventName)
  {
    auto event = (*events)[eventName];

    if (!event)
      return nullptr;

    return event->callbacks;
  }

  /**
   * @brief If we create our own events that fire from within the code
   * we register them with nullptr SinkObj here
   */
  void InitCustom()
  {
    events->emplace("update", new EventState(nullptr));
    events->emplace("tick", new EventState(nullptr));
    events->emplace("browserMessage", new EventState(nullptr));
    events->emplace("consoleMessage", new EventState(nullptr));
    events->emplace("ipcMessage", new EventState(nullptr));
  }

  /**
   * @brief Fetches collections of sinks from corresponding event handlers.
   * Populates global event map with event names, corresponding Sink class
   * instance and empty callback collection, which is being filled when JS
   * plugin subscribes to an event.
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
  }

private:
  EventManager() = default;
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  void ProcessSinks(SinkSet sinkSet)
  {
    if (sinkSet->empty()) {
      return;
    }

    for (const auto& sink : *sinkSet) {
      for (const auto& event : *sink->events) {
        std::vector<std::string_view> linkedEvents;
        // use std::copy_if?
        for (const auto& ev : *sink->events) {
          if (event != ev)
            linkedEvents.push_back(ev);
        }

        auto sinkObj = new SinkObject(sink, &linkedEvents);
        events->emplace(event, new EventState(sinkObj));
      }
    }
  }

  EventMap events;
};
