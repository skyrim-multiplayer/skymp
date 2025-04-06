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

#ifdef ENABLE_SKYRIM_AE
template <class T, class E>
  requires std::same_as<T, RE::PlayerCharacter>
inline RE::BSTEventSource<E>* GetEventSource()
{
  return nullptr;
}
#endif
