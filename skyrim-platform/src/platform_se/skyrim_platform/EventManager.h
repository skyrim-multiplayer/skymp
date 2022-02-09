#pragma once
#include "EventHandler.h"
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

/**
 * TODO: if we want to modularize platform further
 * we should not let EventManager use JsValue here
 * and instead store everything coming from JS side as
 * some generic objects
 *
 * this would require some refactored translation mechanism
 * native->generic->jsValue
 * jsValue->generic->native
 *
 */
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
   * @brief If we create our own custom events
   * that don't have sinks and fire from within the code
   * we register them with nullptr SinkObj here.
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
   * @brief Fetches collection of sinks, populates global event map with event
   * names, corresponding Sink class instance and empty callback collection,
   * which is being filled when JS plugin subscribes to an event.
   */
  void Init()
  {
    if (const auto sinks = EventHandler::GetSingleton()->GetSinks()) {
      if (sinks->empty()) {
        return;
      }

      for (const auto& sink : *sinks) {
        for (const auto& event : sink->events) {
          std::vector<std::string_view> linkedEvents;

          std::copy_if(sink->events.begin(), sink->events.end(),
                       std::back_inserter(linkedEvents),
                       [&](const char* const s) { return s != event; });

          auto sinkObj = new SinkObject(sink, &linkedEvents);
          events->emplace(event, new EventState(sinkObj));
        }
      }
    }
  }

private:
  EventManager() = default;
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  EventMap events;
};
