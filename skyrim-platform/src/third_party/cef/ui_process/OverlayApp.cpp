#include "MyChromiumApp.hpp"

#include <thread>

namespace CEFUtils {
MyChromiumApp::MyChromiumApp(
  const std::function<OverlayRenderProcessHandler*()>& aFactory) noexcept
  : m_pRenderProcess(aFactory())
{
}

CefRefPtr<CefRenderProcessHandler> MyChromiumApp::GetRenderProcessHandler()
{
  return m_pRenderProcess;
}

void ExitCheck(CefRefPtr<CefCommandLine> apArgs)
{
  const auto pid = apArgs->GetSwitchValue("pid");
  const DWORD parentId = std::stoul(pid.c_str());

  Sleep(5000);

  const auto hProcess =
    OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                FALSE, parentId);

  while (WaitForSingleObject(hProcess, INFINITE) == WAIT_TIMEOUT)
    ;

  exit(0);
}

int UIMain(
  const char* acpArgs, const HINSTANCE aInstance,
  const std::function<OverlayRenderProcessHandler*()>& acFactory) noexcept
{
  auto pArgs = CefCommandLine::CreateCommandLine();
  pArgs->InitFromString(acpArgs);

  std::thread t{ ExitCheck, pArgs };
  t.detach();

  CefMainArgs mainArgs(aInstance);
  CefRefPtr<MyChromiumApp> App = new MyChromiumApp(acFactory);

  return CefExecuteProcess(mainArgs, App, nullptr);
}
}
