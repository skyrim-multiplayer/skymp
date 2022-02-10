#include "Hooks.h"
#include "EventsApi.h"

struct OnConsoleVPrint
{
  static void thunk(void* unk1, const char* msg)
  {
    if (msg) {
      EventsApi::SendConsoleMsgEvent(msg);
    }

    func(unk1, msg);
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnConsoleVPrintHook()
{
  REL::Relocation<std::uintptr_t> hook{ REL::ID(51110), 0x300 };
  Hooks::write_thunk_call<OnConsoleVPrint>(hook.address());
}

void Hooks::Install()
{
  InstallOnConsoleVPrintHook();

  logger::info("CommonLib hooks installed.");
}
