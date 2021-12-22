#include "Hooks.h"

struct OnSendEvent
{
  static void thunk(RE::VMHandle a_handle, const FixedString& a_eventName,
                    RE::BSScript::IFunctionArguments* a_args)
  {
    logger::info("Event: {}", a_eventName);

    func(a_handle, a_eventName, a_args);
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnSendEventHook()
{
  REL::Relocation<std::uintptr_t> vtbl{ REL::ID(252631) };
  Hooks::write_vfunc<0x24, OnSendEvent>(vtbl);
}

void Hooks::Install()
{
  /* InstallOnSendEventHook();
  logger::info("CommonLib hooks installed."); */
}
