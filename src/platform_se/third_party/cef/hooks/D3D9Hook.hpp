#pragma once

#include <Signal.hpp>

struct IDirect3D9;
struct IDirect3DDevice9;

namespace CEFUtils
{
    struct D3D9Hook
    {
        TP_NOCOPYMOVE(D3D9Hook);

        Signal<void(IDirect3D9*, IDirect3DDevice9*)> OnCreate;
        Signal<void(IDirect3DDevice9*)> OnPresent;
        Signal<void(IDirect3DDevice9*)> OnReset;

        static void Install() noexcept;
        static D3D9Hook& Get() noexcept;

    private:

        D3D9Hook() noexcept;
        ~D3D9Hook() = default;
    };
}
