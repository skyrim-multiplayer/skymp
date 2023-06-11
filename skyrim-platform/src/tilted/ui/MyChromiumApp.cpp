#include <DInputHook.hpp>
#include <Filesystem.hpp>
#include <MyChromiumApp.h>
#include <filesystem>
#include <fstream>

#include <random>
#include <string>

// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
namespace {
std::string random_string(std::string::size_type length)
{
  static auto& chrs = "0123456789"
                      "abcdefghijklmnopqrstuvwxyz"
                      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  thread_local static std::mt19937 rg{ std::random_device{}() };
  thread_local static std::uniform_int_distribution<std::string::size_type>
    pick(0, sizeof(chrs) - 2);

  std::string s;

  s.reserve(length);

  while (length--)
    s += chrs[pick(rg)];

  return s;
}
}

namespace CEFUtils {
std::string MyChromiumApp::GetCurrentSpToken()
{
  static const auto str = random_string(32);
  return str;
}

MyChromiumApp::MyChromiumApp(
  std::unique_ptr<RenderProvider> apRenderProvider,
  std::shared_ptr<ProcessMessageListener> onProcessMessage_,
  std::wstring aProcessName) noexcept
  : m_pBrowserProcessHandler(new MyBrowserProcessHandler)
  , m_pRenderProvider(std::move(apRenderProvider))
  , onProcessMessage(onProcessMessage_)
  , m_processName(std::move(aProcessName))
{
}

void MyChromiumApp::Initialize() noexcept
{
  if (m_pGameClient)
    return;

  CefMainArgs args(GetModuleHandleA(nullptr));

  const auto currentPath = std::filesystem::current_path();

  CefSettings settings;

  settings.no_sandbox = true;
  settings.multi_threaded_message_loop = true;
  settings.windowless_rendering_enabled = true;

#ifdef DEBUG
  settings.log_severity = LOGSEVERITY_VERBOSE;
#else
  settings.log_severity = LOGSEVERITY_VERBOSE;
  settings.remote_debugging_port = 9000;
#endif

  // We want different CEFTemp paths for the different game installations
  size_t hash =
    std::hash<std::wstring>{}(std::filesystem::current_path().wstring());

  auto ceftempPath = std::filesystem::temp_directory_path() /
    L"Skyrim Platform" / (L"CEFTemp" + std::to_wstring(hash));
  auto logPath = ceftempPath / L"cef_debug.log";

  CefString(&settings.log_file).FromWString(logPath.wstring());
  CefString(&settings.cache_path).FromWString(ceftempPath.wstring());

  CefString(&settings.browser_subprocess_path)
    .FromWString(currentPath / m_processName);

  CefString(&settings.resources_dir_path)
    .FromWString(currentPath / "Data" / "Platform" / "Distribution" / "CEF");
  CefString(&settings.locales_dir_path)
    .FromWString(currentPath / "Data" / "Platform" / "Distribution" / "CEF" /
                 "locales");

  if (!CefInitialize(args, settings, this, nullptr)) {
    MessageBoxA(0,
                "CefInitialize failed (You probably have Skyrim Together "
                "installed, SP isn't compatible with it)",
                "Error", MB_ICONERROR);
  }

  m_pGameClient =
    new OverlayClient(m_pRenderProvider->Create(), onProcessMessage);

  CefBrowserSettings browserSettings{};

  browserSettings.windowless_frame_rate = 60;

  CefWindowInfo info;
  info.SetAsWindowless(m_pRenderProvider->GetWindow());

  if (!CefBrowserHost::CreateBrowser(info, m_pGameClient.get(),
                                     L"file:///Data/Platform/UI/index.html",
                                     browserSettings, nullptr, nullptr)) {

    MessageBoxA(0,
                "CreateBrowser failed (You probably have Skyrim Together "
                "installed, SP isn't compatible with it)",
                "Error", MB_ICONERROR);
  }
}

void MyChromiumApp::ExecuteAsync(
  const std::string& acFunction,
  const CefRefPtr<CefListValue>& apArguments) const noexcept
{
  if (!m_pGameClient)
    return;

  auto pMessage = CefProcessMessage::Create("browser-event");
  auto pArguments = pMessage->GetArgumentList();

  const auto pFunctionArguments =
    apArguments ? apArguments : CefListValue::Create();

  pArguments->SetString(0, acFunction);
  pArguments->SetList(1, pFunctionArguments);

  auto pBrowser = m_pGameClient->GetBrowser();
  if (pBrowser) {
    pBrowser->GetMainFrame()->SendProcessMessage(PID_RENDERER, pMessage);
  }
}

void MyChromiumApp::InjectKey(const cef_key_event_type_t aType,
                              const uint32_t aModifiers, const uint16_t aKey,
                              const uint16_t aScanCode) const noexcept
{
  if (m_pGameClient && m_pGameClient->IsReady()) {
    CefKeyEvent ev;

    ev.type = aType;
    ev.modifiers = aModifiers;
    ev.windows_key_code = aKey;
    ev.native_key_code = aScanCode;

    m_pGameClient->GetBrowser()->GetHost()->SendKeyEvent(ev);
  }
}

void MyChromiumApp::InjectMouseButton(const uint16_t aX, const uint16_t aY,
                                      const cef_mouse_button_type_t aButton,
                                      const bool aUp,
                                      const uint32_t aModifier) const noexcept
{
  if (m_pGameClient && m_pGameClient->IsReady()) {
    CefMouseEvent ev;

    ev.x = aX;
    ev.y = aY;
    ev.modifiers = aModifier;

    m_pGameClient->GetBrowser()->GetHost()->SendMouseClickEvent(ev, aButton,
                                                                aUp, 1);
  }
}

void MyChromiumApp::InjectMouseMove(const float aX, const float aY,
                                    const uint32_t aModifier,
                                    bool isBrowserFocused) const noexcept
{
  std::string url;
  {
    std::lock_guard l(share.m);
    url = share.url;
  }

  if (m_pGameClient && m_pGameClient->IsReady()) {
    thread_local clock_t g_lastExecute = 0;
    if (clock() - g_lastExecute > CLOCKS_PER_SEC) {
      g_lastExecute = clock();
      auto script = "window.spBrowserToken = '" + GetCurrentSpToken() + "';";

      bool urlChanged = false;
      {
        std::lock_guard l(share2.m);
        std::swap(urlChanged, share2.urlChanged);
      }

      if (urlChanged) {
        if (url.size() > 0) {
          m_pGameClient->GetBrowser()->GetMainFrame()->LoadURL(url);
        } else {
          m_pGameClient->GetBrowser()->GetMainFrame()->LoadURL(
            "file:///Data/Platform/UI/index.html");
        }
      }
      m_pGameClient->GetBrowser()->GetMainFrame()->ExecuteJavaScript(
        script, "my mind", 0);
    }

    CefMouseEvent ev;

    ev.x = aX;
    ev.y = aY;
    ev.modifiers = aModifier;

    m_pGameClient->GetMyRenderHandler()->SetCursorLocation(aX, aY);

    if (isBrowserFocused && aX >= 0 && aY >= 0)
      m_pGameClient->GetBrowser()->GetHost()->SendMouseMoveEvent(ev, false);
  }
}

void MyChromiumApp::InjectMouseWheel(const uint16_t aX, const uint16_t aY,
                                     const int16_t aDelta,
                                     const uint32_t aModifier) const noexcept
{
  if (m_pGameClient && m_pGameClient->IsReady()) {
    CefMouseEvent ev;

    ev.x = aX;
    ev.y = aY;
    ev.modifiers = aModifier;

    m_pGameClient->GetBrowser()->GetHost()->SendMouseWheelEvent(ev, 0, aDelta);
  }
}

void MyChromiumApp::ExecuteJavaScript(const std::string& src) const noexcept
{
  if (m_pGameClient && m_pGameClient->IsReady()) {
    m_pGameClient->GetBrowser()->GetMainFrame()->ExecuteJavaScript(
      src, "my mind", 0);
  }
}

bool MyChromiumApp::LoadUrl(const char* url) noexcept
{
  {
    std::lock_guard l(share.m);
    share.url = url;
  }
  {
    std::lock_guard l(share2.m);
    share2.urlChanged = true;
  }
  return true;
}

void MyChromiumApp::RunTasks()
{
  if (m_pGameClient && m_pGameClient->IsReady()) {
    bool isBrowserFocused = CEFUtils::DInputHook::ChromeFocus();

    int isFocusedInt = isBrowserFocused ? 1 : 0;
    if (isFocusedInt != m_wasFocused) {
      m_wasFocused = isFocusedInt;
      m_pGameClient->GetBrowser()->GetHost()->SetFocus(isBrowserFocused);
    }
  }
}

void MyChromiumApp::OnBeforeCommandLineProcessing(
  const CefString& aProcessType, CefRefPtr<CefCommandLine> aCommandLine)
{
}
}
