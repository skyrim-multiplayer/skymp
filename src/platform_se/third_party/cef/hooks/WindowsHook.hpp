#pragma once

#include <Windows.h>

#include <Meta.hpp>

namespace CEFUtils
{
    struct WindowsHook
    {
        TP_NOCOPYMOVE(WindowsHook);

        void SetCallback(WNDPROC apCallback) noexcept { m_pCallback = apCallback; }
        [[nodiscard]] WNDPROC GetCallback() const noexcept { return m_pCallback; }

        static void Install() noexcept;
        static WindowsHook& Get() noexcept;

    private:

        WindowsHook() noexcept = default;
        ~WindowsHook() noexcept = default;

        WNDPROC m_pCallback{ nullptr };
    };
}
