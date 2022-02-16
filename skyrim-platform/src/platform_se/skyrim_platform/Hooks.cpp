#include "Hooks.h"
#include "EventHandler.h"

/**
 * @brief This hooks into the game main cycle
 * which behaves much like "our" tick cycle
 * but with a slight artificial delay between ticks.
 * Is mostly used for testing atm.
 */
struct OnUpdate
{
  static void thunk() { func(); };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnUpdateHook()
{
  REL::Relocation<std::uintptr_t> hook{ REL::ID(36564), 0x6e };
  Hooks::write_thunk_call<OnUpdate>(hook.address());
}

struct OnConsoleVPrint
{
  static void thunk(void* unk1, const char* msg)
  {
    if (msg) {
      EventHandler::SendEventConsoleMsg(msg);
    }

    func(unk1, msg);
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnConsoleVPrintHook()
{ // 50180
#ifdef SKYRIMSE
  REL::Relocation<std::uintptr_t> hook{ REL::ID(50180), 0x163 };
#else
  REL::Relocation<std::uintptr_t> hook{ REL::ID(51110), 0x300 };
#endif
  Hooks::write_thunk_call<OnConsoleVPrint>(hook.address());
}

void Hooks::Install()
{
  // InstallOnUpdateHook();
  InstallOnConsoleVPrintHook();

  logger::info("CommonLib hooks installed.");
}
