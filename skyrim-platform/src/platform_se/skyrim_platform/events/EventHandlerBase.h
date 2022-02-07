#pragma once
#include "EventUtils.h"

/**
 * concepts to distinguish what approach we should use to register sinks
 */
template <class T>
concept HasEvent = requires
{
  typename T::Event;
};

template <class T, class E>
concept SingletonSource = requires
{
  {
    T::GetSingleton()
    } -> std::convertible_to<RE::BSTEventSource<E>*>;
};

/**
 * @brief Each instance should be able to manipulate its own sink.
 */
struct Sink
{
  Sink(EventHandlerBase* _handler, std::vector<const char*>* _events,
       std::function<void(::Sink*)> _add, std::function<void(::Sink*)> _remove,
       std::function<bool(::Sink*)> _validate)
    : Activate(_add)
    , Deactivate(_remove)
    , IsActive(_validate)
    , events(_events)
    , handler(_handler)
  {
  }
  EventHandlerBase* handler;
  std::vector<const char*>* events;

  std::function<void(::Sink*)> Activate;
  std::function<void(::Sink*)> Deactivate;
  std::function<bool(::Sink*)> IsActive;
};

using SinkSet = robin_hood::unordered_set<const ::Sink*>*;
using EventResult = RE::BSEventNotifyControl;

class EventHandlerBase
{
public:
  virtual EventHandlerBase* GetSingleton();

  /**
   * @brief Fetches event map from event handlers.
   * Returns std::unordered_map<std::vector<const char*>*, const Sink*>*
   */
  SinkSet GetSinks() { return sinks; }

  /**
   * @brief Create new sink instance and add it to sink set.
   * Registration via specific source holder.
   * requires T::Singleton = BSTEventSource<E>
   */
  template <class T, class E>
  requires SingletonSource<T, E>
  void AppendSink(std::vector<const char*>* eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      GetSingleton(), // handler
      eventNames,
      // Activate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->AddActiveSink<T, E>(sink);
      },
      // Deactivate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->RemoveActiveSink<T, E>(sink);
      },
      // IsActive
      [](EventHandlerBase* handler, const ::Sink* sink) -> bool {
        return handler->IsActiveSink(sink);
      });

    sinks->emplace(sink);
  }

  /**
   * @brief Create new sink instance and add it to sink set.
   * Registration via script source holder.
   */
  template <class E>
  void AppendSink(std::vector<const char*>* eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      GetSingleton(), // handler
      eventNames,
      // Activate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->AddActiveSink<E>(sink);
      },
      // Deactivate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->RemoveActiveSink<E>(sink);
      },
      // IsActive
      [](EventHandlerBase* handler, const ::Sink* sink) -> bool {
        return handler->IsActiveSink(sink);
      });

    sinks->emplace(sink);
  }

  /**
   * @brief Create new sink instance and add it to sink set.
   * Registration via T::GetEventSource() holder.
   * requires T::GetEventSource = BSTEventSource<T::Event>
   */
  template <HasEvent T>
  void AppendSink(std::vector<const char*>* eventNames)
  {
    // should consider checking if sink exists
    // but since we store sink pointers
    // the only option it to loop through all sinks
    // and check for event names, TODO?

    auto sink = new Sink(
      GetSingleton(), // handler
      eventNames,
      // Activate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->AddActiveSink<T, E>(sink);
      },
      // Deactivate
      [](EventHandlerBase* handler, const ::Sink* sink) {
        handler->RemoveActiveSink<T, E>(sink);
      },
      // IsActive
      [](EventHandlerBase* handler, const ::Sink* sink) -> bool {
        return handler->IsActiveSink(sink);
      });

    sinks->emplace(sink);
  }

  bool IsActiveSink(const Sink* sink) { return activeSinks->contains(sink); }

  /**
   * @brief Registers sink using specific event source holder.
   */
  template <class T, class E>
  requires SingletonSource<T, E>
  void AddActiveSink(const Sink* sink)
  {
    if (const RE::BSTEventSource<E>* holder = T::GetSingleton()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
      activeSinks->emplace(sink);
    }
  }

  /**
   * @brief Registers sink using ScriptEventSourceHolder.
   */
  template <class E>
  void AddActiveSink(const Sink* sink)
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<E>(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
      activeSinks->emplace(sink);
    }
  }

  /**
   * @brief Registers sink using T::GetEventSource() holder.
   * Used by story events.
   */
  template <HasEvent T>
  void AddActiveSink(const Sink* sink)
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(T::Event).name());
      activeSinks->emplace(sink);
    }
  }

  /**
   * @brief Unregisters sink using specific event source holder.
   */
  template <class T, class E>
  requires SingletonSource<T, E>
  void RemoveActiveSink(const Sink* sink)
  {
    if (const RE::BSTEventSource<E>* holder = T::GetSingleton()) {
      holder->RemoveEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks->erase(sink);
    }
  }

  /**
   * @brief Unregisters sink using ScriptEventSourceHolder.
   */
  template <class E>
  void RemoveActiveSink(const Sink* sink)
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->RemoveEventSink<E>(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks->erase(sink);
    }
  }

  /**
   * @brief Unregisters sink using T::GetEventSource() holder.
   * Used by story events.
   */
  template <HasEvent T>
  void RemoveActiveSink(const Sink* sink)
  {
    if (const auto holder = T::GetEventSource()) {
      holder->RemoveEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
      activeSinks->erase(sink);
    }
  }

protected:
  SinkSet sinks;
  SinkSet activeSinks;
};
