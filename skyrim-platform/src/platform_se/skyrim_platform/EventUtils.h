#pragma once

template <class E>
inline RE::BSTEventSource<E>* GetEventSourceScriptEvent()
{
  return RE::ScriptEventSourceHolder::GetSingleton()->GetEventSource<E>();
}

template <class T, class E = T::Event>
  requires HasEvent<T>
inline RE::BSTEventSource<E>* GetEventSource()
{
  return T::GetEventSource();
}

template <class T, class E>
  requires SingletonSource<T, E>
inline RE::BSTEventSource<E>* GetEventSource()
{
  return T::GetSingleton();
}

// helper functions

template <class T>
inline std::shared_ptr<T> CopyEventPtr(const T* ptr)
{
  return std::make_shared<T>(*ptr);
}
