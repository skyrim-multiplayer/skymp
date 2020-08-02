#include <MyCtxHandler.hpp>

namespace CEFUtils
{
    void MyCtxHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
    {
        model->Clear();
    }
}
