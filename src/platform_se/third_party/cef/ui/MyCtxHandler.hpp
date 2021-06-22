#pragma once
#include <include/cef_context_menu_handler.h>

#include <Meta.hpp>

namespace CEFUtils
{
    struct MyCtxHandler final : CefContextMenuHandler
    {
        MyCtxHandler() = default;
        virtual ~MyCtxHandler() = default;

        TP_NOCOPYMOVE(MyCtxHandler);

        void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;

        IMPLEMENT_REFCOUNTING(MyCtxHandler);
    };
}
