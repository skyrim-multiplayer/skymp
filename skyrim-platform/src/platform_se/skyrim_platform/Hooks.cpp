#include "Hooks.h"
#include "EventHandler.h"

/**
 * @brief This hooks into the game main cycle
 * which behaves much like "our" tick cycle
 * but with a slight artificial delay between ticks.
 * Is mostly used for testing atm.
 */
struct OnFrameUpdate
{
  static void thunk(std::int64_t unk) { func(unk); };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnFrameUpdateHook()
{
  Hooks::write_thunk_call<OnFrameUpdate>(
    Offsets::Hooks::FrameUpdate.address());
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
{
  Hooks::write_thunk_call<OnConsoleVPrint>(Offsets::Hooks::VPrint.address());
}

void Hooks::Install()
{
  // InstallOnFrameUpdateHook();
  InstallOnConsoleVPrintHook();

  logger::info("CommonLib hooks installed.");
}
