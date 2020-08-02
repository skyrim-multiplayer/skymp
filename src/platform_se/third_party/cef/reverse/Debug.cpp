#include <Debug.hpp>
#include <thread>

#include <windows.h>

namespace CEFUtils
{
    void Debug::WaitForDebugger() noexcept
    {
        while (!IsDebuggerPresent())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void Debug::CreateConsole() noexcept
    {
        if (AllocConsole())
        {
            freopen("CONOUT$", "w", stdout);
            SetConsoleTitleA("Tilted Reverse Console");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
        }
    }
}
