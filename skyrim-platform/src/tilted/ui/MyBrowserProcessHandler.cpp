#include <MyBrowserProcessHandler.h>

namespace CEFUtils {

void MyBrowserProcessHandler::OnBeforeChildProcessLaunch(
    CefRefPtr<CefCommandLine> command_line) {
  // Set the process ID
  command_line->AppendSwitchWithValue("pid", std::to_string(GetCurrentProcessId()));

  command_line->AppendSwitch("enable-media-stream");
  command_line->AppendSwitch("use-fake-ui-for-media-stream");
}

}  // namespace CEFUtils
