#pragma once

namespace CEFUtils
{
    struct Debug
    {
        static void WaitForDebugger() noexcept;
        static void CreateConsole() noexcept;
    };
}
