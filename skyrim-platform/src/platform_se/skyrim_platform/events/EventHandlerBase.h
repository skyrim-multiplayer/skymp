#pragma once
#include "EventUtils.h"

/**
 * @brief Each instance should be able to manipulate its own sink,
 * so we dont have to rely on handlers to do that.
 */
struct Sink
{
  Sink(std::function<void()> _add, std::function<void()> _remove,
       std::function<bool()> _validate)
    : Activate(_add)
    , Deactivate(_remove)
    , IsActive(_validate)
  {
  }
  std::function<void()> Activate;
  std::function<void()> Deactivate;
  std::function<bool()> IsActive;
};

using EventMap = std::unordered_map<std::vector<const char*>*, const Sink*>*;
using EventResult = RE::BSEventNotifyControl;

class EventHandlerBase
{
public:
  virtual EventHandlerBase* GetSingleton();

  /**
   * @brief Fetches event map from event handlers.
   * Returns std::unordered_map<std::vector<const char*>*, const Sink*>*
   */
  EventMap FetchEvents() { return sinks; }

protected:
  EventMap sinks;
  std::unordered_set<std::vector<const char*>*>* activeSinks;

  /**
   * @brief Check if activeSinks set contains event name,
   * meaning sink is active.
   * @param events ptr to vector with event names
   */
  bool IsActiveSink(std::vector<const char*>* events)
  {
    return activeSinks->contains(events);
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding vector of event names x_sink<E>()
   */
  template <class E>
  void AppendSink(std::vector<const char*>* events)
  {
    if (sinks->contains(events)) {
      logger::critical(
        "Attempt to append EventSink for {} failed. Already exists.",
        typeid(E).name());

      return;
    }

    auto sink = new Sink(
      [] {
        add_sink<E>();
        activeSinks->emplace(events);
      },
      [] {
        remove_sink<E>();
        activeSinks->erase(events);
      },
      [](bool) { return IsActiveSink(events); });

    sinks->emplace(events, sink);
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding vector of event names. x_sink<T, T::Event>()
   *
   * There might be a way to validate T::Event with c++20 concepts
   * so we dont have to pass second class
   */
  template <class T, class E>
  void AppendSink(std::vector<const char*>* events)
  {
    if (sinks->contains(events)) {
      logger::critical(
        "Attempt to append EventSink for {} failed. Already exists.",
        typeid(E).name());

      return;
    }

    auto sink = new Sink(
      [&](bool) {
        add_sink<T, E>();
        activeSinks->emplace(events);
      },
      [&](bool) {
        remove_sink<T, E>();
        activeSinks->erase(events);
      },
      [&](bool) { return IsActiveSink(events); });

    sinks->emplace(events, sink);
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding vector of event names x_sink(holder)
   */
  template <class E>
  void AppendSink(std::vector<const char*>* events,
                  RE::BSTEventSource<E>* holder)
  {
    if (sinks->contains(events)) {
      logger::critical(
        "Attempt to append EventSink for {} failed. Already exists.",
        typeid(E).name());

      return;
    }

    auto sink = new Sink(
      [&](bool) {
        add_sink(holder);
        activeSinks->emplace(events);
      },
      [&](bool) {
        remove_sink(holder);
        activeSinks->erase(events);
      },
      [&](bool) { return IsActiveSink(events); });

    sinks->emplace(events, sink);
  }

  /**
   * @brief Registers sink using specific event source holder.
   */
  template <class E>
  static void add_sink(RE::BSTEventSource<E>* holder)
  {
    holder->AddEventSink(GetSingleton());
    logger::debug("Registered {} handler"sv, typeid(E).name());
  }

  /**
   * @brief Registers sink using ScriptEventSourceHolder.
   */
  template <class E>
  static void add_sink()
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<E>(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
    }
  }

  /**
   * @brief Registers sink using T::GetEventSource() holder.
   * Used by story events.
   */
  template <class T, class E>
  static void add_sink()
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
    }
  }

  /**
   * @brief Unregisters sink using specific event source holder.
   */
  template <class E>
  static void remove_sink(RE::BSTEventSource<E>* holder)
  {
    holder->AddEventSink(GetSingleton());
    logger::debug("Unregistered {} handler"sv, typeid(E).name());
  }

  /**
   * @brief Unregisters sink using ScriptEventSourceHolder.
   */
  template <class E>
  static void remove_sink()
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<E>(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
    }
  }

  /**
   * @brief Unregisters sink using T::GetEventSource() holder.
   * Used by story events.
   */
  template <class T, class E>
  static void remove_sink()
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
    }
  }
};
