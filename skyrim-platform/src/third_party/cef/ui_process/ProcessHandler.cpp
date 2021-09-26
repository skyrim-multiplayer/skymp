#include "ProcessHandler.h"

ProcessHandler::ProcessHandler() noexcept
  : OverlayRenderProcessHandler("skyrimPlatform")
{
}

void ProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefV8Context> context)
{
  OverlayRenderProcessHandler::OnContextCreated(browser, frame, context);

  m_pCoreObject->SetValue(
    "sendMessage",
    CefV8Value::CreateFunction("sendMessage", m_pOverlayHandler),
    V8_PROPERTY_ATTRIBUTE_NONE);
}

void ProcessHandler::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame,
                                       CefRefPtr<CefV8Context> context)
{
  OverlayRenderProcessHandler::OnContextReleased(browser, frame, context);
}
