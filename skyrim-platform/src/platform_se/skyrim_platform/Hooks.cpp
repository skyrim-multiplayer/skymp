#include "Hooks.h"
#include "EventHandler.h"
#include <mutex>

#include "SkyretkRTTI.h"

namespace skyretk {

}

namespace hook::internal {

uintptr_t GetAllocationBase(void* ptr)
{
  MEMORY_BASIC_INFORMATION mbi{};
  if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
    return reinterpret_cast<uintptr_t>(mbi.AllocationBase);
  }
  return 0;
}

std::mutex g_mutex;
std::vector<std::tuple<std::string, std::string, RE::BSScript::IFunction*,
                       uintptr_t, uintptr_t>>
  g_boundNatives;
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

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* thisArg,
                      RE::BSScript::IFunction* func);

decltype(&BindNativeMethod) _BindNativeMethod;

void HookVirtualMachineBind()
{
  spdlog::info("Hooking VirtualMachine::Bind");
  REL::Relocation<std::uintptr_t> Vtbl{
    RE::BSScript::Internal::VirtualMachine::VTABLE[0]
  };
  _BindNativeMethod = reinterpret_cast<decltype(_BindNativeMethod)>(
    Vtbl.write_vfunc(0x18, BindNativeMethod));
}

void BindNativeMethod(RE::BSScript::Internal::VirtualMachine* thisArg,
                      RE::BSScript::IFunction* func)
{
  while (!IsDebuggerPresent()) {

    Sleep(1);
  }

  std::stringstream memory;

  for (int i = 0; i < 100; i++) {
    uint8_t* funcPtr = reinterpret_cast<uint8_t*>(func);
    if (i % 8 == 0) {
      memory << std::hex << '[' << (int)i << ']' << ' ';
    }
    memory << std::hex << (int)funcPtr[i] << ' ';
  }

  uint8_t* raw = reinterpret_cast<uint8_t*>(func);
  uintptr_t realFunc = *reinterpret_cast<uintptr_t*>(raw + 0x50);

  uintptr_t moduleBase =
    hook::internal::GetAllocationBase(reinterpret_cast<void*>(realFunc));
  uintptr_t funcOffset = realFunc - moduleBase;

  uintptr_t moduleBaseSkyrimExe =
    reinterpret_cast<uint64_t>(GetModuleHandle(NULL));

  if (moduleBase == moduleBaseSkyrimExe) {
    DumpObjectClassHierarchy(*(uint64_t**)func, false, moduleBaseSkyrimExe);
  }

  const char* funcName = func ? func->GetName().data() : "<null func>";
  const char* className =
    func ? func->GetObjectTypeName().data() : "<null IFunction>";
  spdlog::trace("VirtualMachine::Bind called {} {} {} funcOffset={:x}, "
                "realFunc={:x}, moduleBase={:x}",
                className, funcName, memory.str(), funcOffset, realFunc,
                moduleBase);

  if (func) {
    std::lock_guard<std::mutex> lock(hook::internal::g_mutex);
    hook::internal::g_boundNatives.push_back(
      { className, funcName, func, moduleBase, funcOffset });
  }

  _BindNativeMethod(thisArg, func);
}

std::vector<std::tuple<std::string, std::string, RE::BSScript::IFunction*,
                       uintptr_t, uintptr_t>>
Hooks::GetBoundNatives()
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
