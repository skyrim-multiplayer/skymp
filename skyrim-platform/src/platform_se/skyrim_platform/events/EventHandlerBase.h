#pragma once

/**
 * @brief Each instance should possess functions to manipulate unique sink, so
 * we dont have to rely on handlers to do that
 */
struct Sink
{
  Sink(std::function<bool()> _add, std::function<bool()> _remove,
       std::function<bool()> _validate)
    : Add(_add)
    , Remove(_remove)
    , IsActive(_validate)
  {
  }
  std::function<bool()> Add;
  std::function<bool()> Remove;
  std::function<bool()> IsActive;
};

using EventMap = std::unordered_map<std::vector<const char*>*, const Sink*>*;

class EventHandlerBase
{
public:
  virtual EventHandlerBase* GetSingleton();

  virtual EventMap FetchEvents();

protected:
  EventMap sinks;
  std::unordered_set<std::vector<const char*>*>* activeSinks;

  /**
   * @brief Check if activeSinks set contains event name, meaning sink is
   * active.
   * @param events vector ptr with event names
   */
  bool IsActiveSink(std::vector<const char*>* events)
  {
    return activeSinks->contains(events);
  }

  /**
   * @brief Add event name for activated sink to activeSinks set.
   * @param events vector ptr with event names
   */
  bool AddActiveSink(std::vector<const char*>* events)
  {
    activeSinks->emplace(events);
    return true;
  }

  /**
   * @brief Remove event name for deactivated sink from activeSinks set.
   * @param events vector ptr with event names
   */
  bool RemoveActiveSink(std::vector<const char*>* events)
  {
    activeSinks->erase(events);
    return true;
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding event name. x_sink<E>()
   */
  template <class E>
  bool AppendSink(std::vector<const char*>* events)
  {
    if (sinks->contains(events))
      return false;

    auto sink = new Sink(
      [&](bool) {
        if (IsActiveSink(events))
          return false;

        add_sink<E>();
        return AddActiveSink(events);
      },
      [&](bool) {
        if (!IsActiveSink(events))
          return false;

        remove_sink<E>();
        return RemoveActiveSink(events);
      },
      [&](bool) { return IsActiveSink(events); });
    sinks->emplace(events, sink);
    return true;
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding event name. x_sink<T, T::Event>()
   */
  template <class T, class E = decltype(&T::Event)>
  bool AppendSink(std::vector<const char*>* events)
  {
    if (sinks->contains(events))
      return false;

    auto sink = new Sink(
      [&](bool) {
        if (IsActiveSink(events))
          return false;

        add_sink<T, E>();
        return AddActiveSink(events);
      },
      [&](bool) {
        if (!IsActiveSink(events))
          return false;

        remove_sink<T, E>();
        return RemoveActiveSink(events);
      },
      [&](bool) { return IsActiveSink(events); });
    sinks->emplace(events, sink);
    return true;
  }

  /**
   * @brief Create new Sink class instance and add it to sinks map with
   * corresponding event name. x_sink(holder)
   */
  template <class T = RE::BSTEventSource<class E>>
  bool AppendSink(std::vector<const char*>* events, T* holder)
  {
    if (sinks->contains(events))
      return false;

    auto sink = new Sink(
      [&](bool) {
        if (IsActiveSink(events))
          return false;

        add_sink(holder);
        return AddActiveSink(events);
      },
      [&](bool) {
        if (!IsActiveSink(events))
          return false;

        remove_sink(holder);
        return RemoveActiveSink(events);
      },
      [&](bool) { return IsActiveSink(events); });
    sinks->emplace(events, sink);
    return true;
  }

  template <class E>
  static void add_sink(RE::BSTEventSource<E>* holder)
  {
    holder->AddEventSink(GetSingleton());
    logger::debug("Registered {} handler"sv, typeid(E).name());
  }

  template <class E>
  static void add_sink()
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<E>(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
    }
  }

  template <class T, class E>
  static void add_sink()
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Registered {} handler"sv, typeid(E).name());
    }
  }

  template <class E>
  static void remove_sink(RE::BSTEventSource<E>* holder)
  {
    holder->AddEventSink(GetSingleton());
    logger::debug("Unregistered {} handler"sv, typeid(E).name());
  }

  template <class E>
  static void remove_sink()
  {
    if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
      holder->AddEventSink<E>(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
    }
  }

  template <class T, class E = decltype(&T::Event)>
  static void remove_sink()
  {
    if (const auto holder = T::GetEventSource()) {
      holder->AddEventSink(GetSingleton());
      logger::debug("Unregistered {} handler"sv, typeid(E).name());
    }
  }
};
