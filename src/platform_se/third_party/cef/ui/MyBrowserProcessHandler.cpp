#include <MyBrowserProcessHandler.hpp>

namespace CEFUtils
{
    void MyBrowserProcessHandler::OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line)
    {
        command_line->AppendSwitchWithValue("pid", std::to_string(GetCurrentProcessId()));
    }
}
