#pragma once

#include <include/cef_browser_process_handler.h>

namespace CEFUtils
{
    struct MyBrowserProcessHandler : CefBrowserProcessHandler
    {
        void OnBeforeChildProcessLaunch(CefRefPtr<CefCommandLine> command_line) override;

        IMPLEMENT_REFCOUNTING(MyBrowserProcessHandler);
    };
}
