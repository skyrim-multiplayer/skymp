#pragma once
#include "JsEngine.h"
#include <RE/MenuControls.h>
#include <RE/UI.h>

namespace UiApi
{
    void replaceMenu(std::string menuName);
    void disableMenu(std::string menuName);

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
            "disableMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                disableMenu(args[1].ToString());
                return JsValue::Undefined();
                })
        );
        exports.SetProperty("skyrimUi", skyrimUi);
    }
}