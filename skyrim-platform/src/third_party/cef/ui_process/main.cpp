#include "MyChromiumApp.hpp"
#include "ProcessHandler.h"
#include <filesystem>
#include <fstream>
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
  std::function<CEFUtils::OverlayRenderProcessHandler*()> f = []() {
    return new ProcessHandler;
  };

  [&]() {
    __try {
      CEFUtils::UIMain(lpCmdLine, hInstance, f);
    } __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
  }();
}
