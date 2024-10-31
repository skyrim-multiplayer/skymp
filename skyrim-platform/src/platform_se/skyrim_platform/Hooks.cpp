#include "Hooks.h"
#include "EventHandler.h"
#include <mutex>

namespace hook::internal {
  std::mutex g_mutex;
  std::vector<RE::BSScript::IFunction*> g_boundNatives;
}

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

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* thisArg, RE::BSScript::IFunction *func);

decltype(&BindNativeMethod) _BindNativeMethod;

void HookVirtualMachineBind()
{
    spdlog::info("Hooking VirtualMachine::Bind");
		REL::Relocation<std::uintptr_t> Vtbl{ RE::BSScript::Internal::VirtualMachine::VTABLE[0] };
		_BindNativeMethod = reinterpret_cast<decltype(_BindNativeMethod)>(Vtbl.write_vfunc(0x18, BindNativeMethod));
}

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* thisArg, RE::BSScript::IFunction *func)
{
    const char *funcName = func ? func->GetName().data() : "<null func>";
    const char *className = func ? func->GetObjectTypeName().data() : "<null IFunction>";
    spdlog::trace("VirtualMachine::Bind called {} {}", className, funcName);

    if (func) {
      std::lock_guard<std::mutex> lock(hook::internal::g_mutex);
      hook::internal::g_boundNatives.push_back(func);
    }

		_BindNativeMethod(thisArg, func);
}

std::vector<RE::BSScript::IFunction *> Hooks::GetBoundNatives()
{
  std::lock_guard<std::mutex> lock(hook::internal::g_mutex);
  return hook::internal::g_boundNatives;
}

void Hooks::Install()
{
  // InstallOnFrameUpdateHook();
  InstallOnConsoleVPrintHook();
  HookVirtualMachineBind();

  logger::info("CommonLib hooks installed.");
}
