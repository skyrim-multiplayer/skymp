#include "EventManager.h"
#include "EventsApi.h"
#include "InvalidArgumentException.h"

/**
 * @brief If we create our own custom events
 * that don't have sinks and fire from within the code
 * we register them with nullptr SinkObj here.
 */
void EventManager::InitCustom()
{
  auto manager = GetSingleton();

  manager->EmplaceEvent("update");
  manager->EmplaceEvent("tick");
  manager->EmplaceEvent("browserMessage");
  manager->EmplaceEvent("consoleMessage");
  manager->EmplaceEvent("ipcMessage");

  // skse messages
  manager->EmplaceEvent("skyrimLoaded");
  manager->EmplaceEvent("newGame");
  manager->EmplaceEvent("preLoadGame");
  manager->EmplaceEvent("postLoadGame");
  manager->EmplaceEvent("saveGame");
  manager->EmplaceEvent("deleteGame");

  logger::debug("Custom events initialized.");
}

/**
 * @brief Fetches collection of sinks, populates global event map with event
 * names, corresponding Sink class instance and empty callback collection,
 * which is being filled when JS plugin subscribes to an event.
 */
void EventManager::Init()
{
  if (const auto sinks = EventHandler::GetSingleton()->GetSinks()) {
    if (sinks->empty()) {
      return;
    }

    auto manager = GetSingleton();

    for (const Sink* sink : *sinks) {
      for (const char* event : sink->events) {
        std::vector<std::string> linkedEvents;
        linkedEvents.reserve(sink->events.size() - 1);

        std::copy_if(sink->events.begin(), sink->events.end(),
                     std::back_inserter(linkedEvents),
                     [&](const char* s) { return strcmp(s, event) != 0; });

        SinkObject sinkObj{ sink, linkedEvents };
        manager->EmplaceEvent(event, EventState{ sinkObj });
      }
    }
  }

  logger::debug("Game events initialized.");
}

std::unique_ptr<EventHandle> EventManager::Subscribe(
  const std::string& eventName,
  const std::shared_ptr<Napi::Reference<Napi::Function>>& callback,
  bool runOnce)
{
  auto it = events.find(eventName);
  if (it == events.end()) {
    logger::critical("Subscription to event failed. "
                     "{} is not a valid argument for eventName",
                     eventName);
    throw InvalidArgumentException("eventName", eventName);
  }

  EventState& state = it->second;

  // If sink for that event is not active, activate it
  if (state.sinkObj) {
    const Sink* sink = state.sinkObj->GetSink();

    if (!sink->IsActive(sink)) {
      sink->Activate(sink);
      logger::debug("Activated sink for event {}", eventName);
    }
  }

  auto cb = CallbackObject(callback, runOnce);
  auto uid = this->nextUid++;

  state.callbacks.emplace(uid, cb);

  logger::debug("Subscribed to event {}, callback uid {}", eventName, uid);

  return std::make_unique<EventHandle>(uid, eventName);
}

void EventManager::Unsubscribe(uintptr_t uid, const std::string& eventName)
{
  auto it = events.find(eventName);
  if (it == events.end()) {
    logger::info("Unsubscribe attempt failed, event {} not found", eventName);
    return;
  }
  EventState& state = it->second;

  const size_t numDeletions = state.callbacks.erase(uid);
  if (numDeletions == 0) {
    logger::info("Unsubscribe attempt failed, callback with uid {} for event "
                 "{} not found",
                 uid, eventName);
    return;
  }

  logger::debug("Unsubscribed from event {}, callback uid {}", eventName, uid);

  // now we need to see if we can deactivate event sink
  DeactivateEventSinkIfNeeded(state, eventName);
}

void EventManager::ClearCallbacks()
{
  for (auto& [eventName, state] : events) {
    state.callbacks.clear();
  }

  auto handler = EventHandler::GetSingleton();
  handler->DeactivateAllSinks();
}

const CallbackObjMap& EventManager::GetCallbackObjMap(const char* eventName)
{
  static const CallbackObjMap kEmptyMap;

  auto it = events.find(eventName);

  return it != events.end() ? it->second.callbacks : kEmptyMap;
}

void EventManager::EmplaceEvent(const std::string& name,
                                const EventState& state)
{
  events.emplace(name, state);
}

EventMap& EventManager::GetEventMap()
{
  return events;
}

void EventManager::DeactivateEventSinkIfNeeded(EventState& state,
                                               const std::string& eventName)
{
  // Event without a sink (custom event), nothing to deactivate
  if (state.sinkObj == std::nullopt) {
    return;
  }

  // Non-empty callbacks, sink still needed
  if (!state.callbacks.empty()) {
    return;
  }

  logger::trace("No other callbacks found for event {}", eventName);

  const Sink* sink = state.sinkObj->GetSink();
  auto& linkedEvents = state.sinkObj->GetLinkedEvents();

  for (const auto& linkedEventName : linkedEvents) {
    logger::trace("Checking callbacks for linked event {}", linkedEventName);

    auto linkedEventNameIt = events.find(linkedEventName);
    if (linkedEventNameIt != events.end() &&
        !linkedEventNameIt->second.callbacks.empty()) {
      logger::trace("Found callbacks for linked event {}. Aborting sink "
                    "deactivation.",
                    linkedEventName);
      // Non-empty callbacks, sink still needed
      return;
    }
  }

  sink->Deactivate(sink);
  logger::debug("Deactivated sink for event {}", eventName);
}
