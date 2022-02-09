#include "EventManager.h"
#include "EventsApi.h"

EventHandle* EventManager::Subscribe(std::string eventName, JsValue callback,
                                     bool runOnce)
{
  // check if event is supported
  auto event = events[eventName];
  if (!event) {
    logger::critical("Subscription to event failed. "
                     "{} is not a valid argument for eventName",
                     eventName);
    throw InvalidArgumentException("eventName", eventName);
    return new EventHandle(0, "");
  }

  // if sink for that event is not active activate it, duh
  if (event->sinkObj) {
    auto sink = event->sinkObj->sink;

    if (!sink->IsActive(sink)) {
      sink->Activate(sink);
    }
  }

  // use callback pointer as unique id for that callback
  auto uid = reinterpret_cast<uintptr_t>(&callback);
  event->callbacks->emplace(uid, new CallbackObject(&callback, runOnce));

  return new EventHandle(uid, eventName);
}

void EventManager::Unsubscribe(uintptr_t uid, std::string eventName)
{
  // check for correct event
  auto event = events[eventName];
  if (!event) {
    return;
  }

  // check if callback with provided uid exists
  if (!event->callbacks->contains(uid)) {
    return;
  }

  event->callbacks->erase(uid);

  // now we need to see if we can deactivate event sink

  // ignore this for custom events
  if (event->sinkObj) {
    auto sink = event->sinkObj->sink;
    // check if there are no other callbacks for this event
    if (event->callbacks->empty()) {
      // check if there are any linked events for this sink
      if (event->sinkObj->linkedEvents->empty()) {
        sink->Deactivate(sink);
      } else {
        // check if there are any callbacks for linked events
        auto sinkIsBusy = false;
        for (const auto& eventName : *event->sinkObj->linkedEvents) {
          if (!events[eventName]->callbacks->empty()) {
            sinkIsBusy = true;
            break;
          }
        }

        if (!sinkIsBusy) {
          sink->Deactivate(sink);
        }
      }
    }
  }
}
