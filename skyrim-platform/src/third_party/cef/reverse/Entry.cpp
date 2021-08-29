#include <windows.h>

#include <App.hpp>
#include <Entry.hpp>

#include <mutex>
#include <thread>

#include <FunctionHook.hpp>
#include <Platform.hpp>

static std::shared_ptr<CEFUtils::SKSEPluginBase> g_pApp;

CEFUtils::SKSEPluginBase& CEFUtils::SKSEPluginBase::GetInstance() noexcept
{
  return *g_pApp;
}

using TWinMain = int(__stdcall)(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                                LPSTR lpCmdLine, int nShowCmd);
TWinMain* OriginalWinMain = nullptr;

static int __stdcall HookedWinMain(HINSTANCE hInstance,
                                   HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                                   int nShowCmd)
{
  // Ensure exceptions won't cause our calls to be skipped
  struct ScopedCaller
  {
    ScopedCaller()
    {
      CEFUtils::SKSEPluginBase::GetInstance().BeginMain();

      TP_HOOK_COMMIT
    }
    ~ScopedCaller() { CEFUtils::SKSEPluginBase::GetInstance().EndMain(); }
  };

  ScopedCaller appCaller;

  return OriginalWinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}

static void SetupMainHook()
{
  OriginalWinMain =
    static_cast<TWinMain*>(CEFUtils::SKSEPluginBase::GetInstance().GetMainAddress());
  if (OriginalWinMain == nullptr)
    return;

  TP_HOOK_IMMEDIATE(&OriginalWinMain, HookedWinMain);
}

static std::once_flag s_mainHookCallFlag;

#if TP_PLATFORM_64

using TGetWinmain = char*(__stdcall*)();
using T__crtGetShowWindowMode = short(__stdcall*)();

TGetWinmain OriginalGetWinmain = nullptr;
T__crtGetShowWindowMode Original__crtGetShowWindowMode = nullptr;

char* __stdcall HookGetWinmain()
{
  std::call_once(s_mainHookCallFlag, SetupMainHook);

  return OriginalGetWinmain();
}

short __stdcall Hook__crtGetShowWindowMode()
{
  std::call_once(s_mainHookCallFlag, SetupMainHook);

  return Original__crtGetShowWindowMode();
}

#else

using TGetStartupInfoA = void(__stdcall*)(LPSTARTUPINFO lpStartupInfo);
TGetStartupInfoA OriginalGetStartupInfoA = nullptr;

void __stdcall HookedGetStartupInfoA(LPSTARTUPINFO lpStartupInfo)
{
  std::call_once(s_mainHookCallFlag, SetupMainHook);

  OriginalGetStartupInfoA(lpStartupInfo);
}

#endif

namespace CEFUtils {
BOOL details::ReverseMain(
  HMODULE hModule, DWORD fdwReason, LPVOID lpReserved,
  const std::function<std::unique_ptr<SKSEPluginBase>()>& aAppFactory) noexcept
{
  TP_UNUSED(hModule);
  TP_UNUSED(lpReserved);

  switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
      g_pApp = aAppFactory();
#if TP_PLATFORM_64
      OriginalGetWinmain = reinterpret_cast<TGetWinmain>(
        TP_HOOK_SYSTEM("api-ms-win-crt-runtime-l1-1-0.dll",
                       "_get_narrow_winmain_command_line", HookGetWinmain));
      Original__crtGetShowWindowMode =
        reinterpret_cast<T__crtGetShowWindowMode>(
          TP_HOOK_SYSTEM("msvcr110.dll", "__crtGetShowWindowMode",
                         Hook__crtGetShowWindowMode));
#else
      OriginalGetStartupInfoA =
        reinterpret_cast<TGetStartupInfoA>(TP_HOOK_SYSTEM(
          "kernel32.dll", "GetStartupInfoA", HookedGetStartupInfoA));
#endif

      SKSEPluginBase::GetInstance().Attach();

      TP_HOOK_COMMIT

      break;
    }
    case DLL_PROCESS_DETACH: {
      SKSEPluginBase::GetInstance().Detach();

      break;
    }
    default:
      break;
  }

  return TRUE;
}
}
