#include <D3D9Hook.hpp>

#define CINTERFACE

#include <d3d9.h>

#include <FunctionHook.hpp>
#include <mutex>
#include <intrin.h>

using TReset = HRESULT(WINAPI*)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
using TPresent = HRESULT(WINAPI*)(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion);
using TCreateDevice = HRESULT(WINAPI*)(IDirect3D9* apDirect3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
using TDirect3DCreate9 = IDirect3D9*(WINAPI*)(UINT);

static TReset RealReset = nullptr;
static TPresent RealPresent = nullptr;
static TCreateDevice RealCreateDevice = nullptr;
static TDirect3DCreate9 RealDirect3DCreate9 = nullptr;

namespace CEFUtils
{

    static HRESULT __stdcall HookReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        TP_EMPTY_HOOK_PLACEHOLDER;

        const auto result = RealReset(pDevice, pPresentationParameters);

        D3D9Hook::Get().OnReset(pDevice);

        return result;
    }

    static HRESULT __stdcall HookPresent(IDirect3DDevice9* pDevice, RECT* pSourceRect, RECT* pDestRect, HWND hDestWindowOverride, RGNDATA* pDirtyRegion)
    {
        TP_EMPTY_HOOK_PLACEHOLDER;

        D3D9Hook::Get().OnPresent(pDevice);

        const auto result = RealPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

        return result;
    }

    HRESULT __stdcall HookCreateDevice(IDirect3D9* apDirect3D9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
    {
        TP_EMPTY_HOOK_PLACEHOLDER;

        const auto result = RealCreateDevice(apDirect3D9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);

        const auto pDevice = *ppReturnedDeviceInterface;

        if (RealReset == nullptr)
        {
            RealReset = pDevice->lpVtbl->Reset;
            TP_HOOK_IMMEDIATE(&RealReset, HookReset);
        }

        if (RealPresent == nullptr)
        {
            RealPresent = pDevice->lpVtbl->Present;
            TP_HOOK_IMMEDIATE(&RealPresent, HookPresent);
        }

        D3D9Hook::Get().OnCreate(apDirect3D9, pDevice);

        return result;
    }

    static IDirect3D9* WINAPI HookDirect3DCreate9(UINT SDKVersion)
    {
        IDirect3D9* pIDirect3D9 = RealDirect3DCreate9(SDKVersion);

        if (RealCreateDevice == nullptr)
        {
            RealCreateDevice = pIDirect3D9->lpVtbl->CreateDevice;
            TP_HOOK_IMMEDIATE(&RealCreateDevice, HookCreateDevice);
        }

        return pIDirect3D9;
    }

    D3D9Hook::D3D9Hook() noexcept
    {
    }

    void D3D9Hook::Install() noexcept
    {
        if (RealDirect3DCreate9 == nullptr)
            RealDirect3DCreate9 = reinterpret_cast<TDirect3DCreate9>(TP_HOOK_SYSTEM("d3d9.dll", "Direct3DCreate9", HookDirect3DCreate9));
    }

    D3D9Hook& D3D9Hook::Get() noexcept
    {
        static D3D9Hook s_instance;
        return s_instance;
    }
}
