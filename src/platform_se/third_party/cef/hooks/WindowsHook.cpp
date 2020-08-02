#include <WindowsHook.hpp>

#include <FunctionHook.hpp>

using TRegisterClassA = ATOM(__stdcall*)(const WNDCLASSA*);
using TRegisterClassW = ATOM(__stdcall*)(const WNDCLASSW*);
using TRegisterClassExA = ATOM(__stdcall*)(const WNDCLASSEXA*);
using TRegisterClassExW = ATOM(__stdcall*)(const WNDCLASSEXW*);

TRegisterClassA RealRegisterClassA = nullptr;
TRegisterClassW RealRegisterClassW = nullptr;
TRegisterClassExA RealRegisterClassExA = nullptr;
TRegisterClassExW RealRegisterClassExW = nullptr;

static WNDPROC RealWndProc;

namespace CEFUtils
{

    LRESULT CALLBACK HookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto pCallback = WindowsHook::Get().GetCallback();
        if (pCallback)
        {
            if (pCallback(hwnd, uMsg, wParam, lParam) != 0)
                return 0;
        }

        return RealWndProc(hwnd, uMsg, wParam, lParam);
    }

    ATOM __stdcall HookRegisterClassA(WNDCLASSA* lpWndClass)
    {
        RealWndProc = lpWndClass->lpfnWndProc;
        lpWndClass->lpfnWndProc = HookWndProc;

        return RealRegisterClassA(lpWndClass);
    }

    ATOM __stdcall HookRegisterClassW(WNDCLASSW* lpWndClass)
    {
        RealWndProc = lpWndClass->lpfnWndProc;
        lpWndClass->lpfnWndProc = HookWndProc;

        return RealRegisterClassW(lpWndClass);
    }

    ATOM __stdcall HookRegisterClassExA(WNDCLASSEXA* lpWndClass)
    {
        RealWndProc = lpWndClass->lpfnWndProc;
        lpWndClass->lpfnWndProc = HookWndProc;

        return RealRegisterClassExA(lpWndClass);
    }

    ATOM __stdcall HookRegisterClassExW(WNDCLASSEXW* lpWndClass)
    {
        RealWndProc = lpWndClass->lpfnWndProc;
        lpWndClass->lpfnWndProc = HookWndProc;

        return RealRegisterClassExW(lpWndClass);
    }

    void WindowsHook::Install() noexcept
    {
        TP_HOOK_IAT(RegisterClassA, "user32.dll");
        TP_HOOK_IAT(RegisterClassW, "user32.dll");
        TP_HOOK_IAT(RegisterClassExA, "user32.dll");
        TP_HOOK_IAT(RegisterClassExW, "user32.dll");
    }

    WindowsHook& WindowsHook::Get() noexcept
    {
        static WindowsHook s_instance;
        return s_instance;
    }

}
