#pragma once
#include "JsEngine.h"
#include <RE/MenuControls.h>
#include <RE/UI.h>

namespace UiApi
{
    void replaceMenu(std::string menuName);
    void openMenu(std::string menuName);
    void closeMenu(std::string menuName);

    inline void Register(JsValue& exports)
    {
        auto mc = RE::MenuControls::GetSingleton();
        auto ui = RE::UI::GetSingleton();

        if (!mc || !ui) {
            return;
        }

        auto skyrimUi = JsValue::Object();
        skyrimUi.SetProperty(
            "replaceMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                replaceMenu(args[1].ToString());
                return JsValue::Undefined();
                })
        );
        skyrimUi.SetProperty(
            "openMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                openMenu(args[1].ToString());
                return JsValue::Undefined();
                })
        );
        skyrimUi.SetProperty(
            "closeMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                closeMenu(args[1].ToString());
                return JsValue::Undefined();
                })
        );
        exports.SetProperty("skyrimUi", skyrimUi);
    }
}