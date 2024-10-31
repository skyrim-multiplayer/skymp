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

// struct VirtualMachineHook 
// {
//     static void thunk(RE::BSScript::Internal::VirtualMachine* vm, void* a_f) {

//       func(vm, a_f);
//     }

//     inline static decltype(&thunk) func{ nullptr };
// };

// void HookVirtualMachineBind() {
//     REL::Relocation<std::uintptr_t> vtbl{ RE::BSScript::Internal::VirtualMachine::VTABLE[0] };


//     //VirtualMachineHook::originalFunc = reinterpret_cast<decltype(VirtualMachineHook::Thunk)>(vtbl.read_vfunc(0x18));

//     Hooks::write_vfunc<0x18, VirtualMachineHook>(vtbl);
// }

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* a_this, void *func);

decltype(&BindNativeMethod) _BindNativeMethod;

void HookVirtualMachineBind()
{
    spdlog::info("Hooking VirtualMachine::Bind");
		REL::Relocation<std::uintptr_t> Vtbl{ RE::BSScript::Internal::VirtualMachine::VTABLE[0] };
		_BindNativeMethod = reinterpret_cast<decltype(_BindNativeMethod)>(Vtbl.write_vfunc(0x18, BindNativeMethod));
}

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* a_this, void *func)
{
    spdlog::info("VirtualMachine::Bind called");
		_BindNativeMethod(a_this, func);
}

void Hooks::Install()
{
  // InstallOnFrameUpdateHook();
  InstallOnConsoleVPrintHook();
  HookVirtualMachineBind();

  logger::info("CommonLib hooks installed.");
}
