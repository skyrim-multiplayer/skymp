#pragma once
#include "EventHandler.h"

struct EventHandle
{
  EventHandle(uintptr_t _uid, const std::string& _eventName)
    : uid(_uid)
    , eventName(_eventName)
  {
  }

  const uintptr_t uid;
  const std::string eventName;
};

struct SinkObject
{
  SinkObject(const ::Sink* _sink, std::vector<std::string_view>& _linkedEvents)
    : sink(_sink)
    , linkedEvents(_linkedEvents)
  {
  }
  const ::Sink* sink;
  const std::vector<std::string_view> linkedEvents;
};

struct CallbackObject
{
  CallbackObject(const JsValue& _callback, bool _runOnce)
    : callback(_callback)
    , runOnce(_runOnce)
  {
  }

  JsValue callback = JsValue::Undefined();
  bool runOnce = false;
};

using CallbackObjMap = robin_hood::unordered_map<uintptr_t, CallbackObject>;

struct EventState
{
  explicit EventState(const SinkObject* _sink)
    : sinkObj(_sink)
  {
    callbacks.reserve(5);
  }

  const SinkObject* sinkObj;
  CallbackObjMap callbacks;
};

using EventMap = robin_hood::unordered_map<std::string_view, EventState*>;

/**
 * TODO: if we want to modularize platform further
 * we should not let EventManager use JsValue here
 * and instead store everything coming from JS side as
 * some generic objects
 *
 * this would require some refactored translation mechanism
 * native->generic->jsValue
 * jsValue->generic->native
 */
class EventManager
{
public:
  [[nodiscard]] static EventManager* GetSingleton()
  {
    static EventManager singleton;
    return &singleton;
  }

  static void Init();
  static void InitCustom();

  std::unique_ptr<EventHandle> Subscribe(const std::string& eventName,
                                         const JsValue& callback,
                                         bool runOnce);

  void Unsubscribe(uintptr_t uid, const std::string_view& eventName);

  void ClearCallbacks();

  CallbackObjMap* GetCallbackObjMap(const char* eventName);

  void EmplaceEvent(const std::string_view& name,
                    EventState* state = new EventState(nullptr));

  EventMap* GetEventMap();

private:
  // last i checked we had ~97 events
  EventManager() { events.reserve(100); }
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  EventMap events;

  uintptr_t nextUid = 0;
};
