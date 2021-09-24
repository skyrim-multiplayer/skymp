#include "UiApi.h"
#include "EventsApi.h"
#include "GameEventSinks.h"

#include <RE/MenuControls.h>
#include <RE/UI.h>
#include <RE/Console.h>
#include <RE/ConsoleLog.h>
#include <RE/ButtonEvent.h>
#include <RE/MenuEventHandler.h>
#include <RE/MenuOpenHandler.h>

#include <map>

extern TaskQueue g_taskQueue;

namespace RE {
    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/238e7815d5e7e0c4a26491d15c45197390391d75/include/RE/F/FavoritesHandler.h
    struct FavoritesHandler : public MenuEventHandler
    {
    public:
        inline static constexpr auto RTTI = RTTI_FavoritesHandler;

        virtual ~FavoritesHandler() = default;  // 00

        // add
        virtual bool CanProcess(InputEvent* a_event) = 0;      // 01
        virtual bool ProcessKinect(KinectEvent* a_event) = 0;  // 02
        virtual bool ProcessButton(ButtonEvent* a_event) = 0;  // 05
    };
    static_assert(sizeof(FavoritesHandler) == 0x10);

    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/01374c521e6ab485e5aca4de83a2d2fdc4c9c3c1/include/RE/M/MenuOpenCloseEvent.h
    class MenuOpenCloseEvent
    {
    public:
        // members
        BSFixedString menuName;  // 00
        bool          opening;   // 08
        std::uint8_t  pad09;     // 09
        std::uint16_t pad0A;     // 0A
        std::uint32_t pad0C;     // 0C
    };
    static_assert(sizeof(MenuOpenCloseEvent) == 0x10);

    //missing in CommonLibSSE
    struct ConsoleOpenHandler : public MenuEventHandler
    {
    public:
        inline static constexpr auto RTTI = RTTI_ConsoleOpenHandler;

        virtual ~ConsoleOpenHandler() = default;  // 00

        // add
        virtual bool CanProcess(InputEvent* a_event) = 0;      // 01
        virtual bool ProcessKinect(KinectEvent* a_event) = 0;  // 02
        virtual bool ProcessButton(ButtonEvent* a_event) = 0;  // 05
    };
}

bool onMenuOpenClose(const char * menuName, bool opening) {
    g_taskQueue.AddTask([=] {
        auto obj = JsValue::Object();

        obj.SetProperty("name", JsValue::String(menuName));
        obj.SetProperty("type", JsValue::String( opening ? "open" : "close"));

        EventsApi::SendEvent("menuOpenClose", { JsValue::Undefined(), obj });
    });

    return true;
}


struct PlaceholderMenuHandler : public RE::MenuEventHandler
{
    PlaceholderMenuHandler() {}

    ~PlaceholderMenuHandler() {};

    bool CanProcess(RE::InputEvent* e) override { return false; };

    bool ProcessKinect(RE::KinectEvent* e) override { return false; };

    bool ProcessButton(RE::ButtonEvent* e) override { return false; };
};

//refactoring disableOriginalConsole() -> disableUi("Console")
void disableUi(std::string menuName)
{
    auto lg = RE::ConsoleLog::GetSingleton();
    lg->Print("disable ui - '%s'", menuName);

    /*
    auto mc = RE::MenuControls::GetSingleton();
    
    if (!mc)
        return;

    PlaceholderMenuHandler* consoleOpenHandler = new PlaceholderMenuHandler;
    mc->RemoveHandler(mc->consoleOpenHandler.get());
    mc->AddHandler(consoleOpenHandler);
    mc->consoleOpenHandler = RE::BSTSmartPointer<RE::ConsoleOpenHandler>((RE::ConsoleOpenHandler*)consoleOpenHandler);
    */
}

class MyEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
    ~MyEventSink() {};
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* e, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override {
        onMenuOpenClose(e->menuName.c_str(), e->opening);

        return RE::BSEventNotifyControl::kContinue;
    };
};

namespace UiApi
{
    void Register(JsValue& exports)
    {
        auto lg = RE::ConsoleLog::GetSingleton();
        auto mc = RE::MenuControls::GetSingleton();
        auto ui = RE::UI::GetSingleton();

        if (!lg || !mc || !ui)
            return;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(new MyEventSink);

        auto uiObj = JsValue::Object();
        uiObj.SetProperty(
            "disableUi",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                disableUi(args[1].ToString());
                return JsValue::Undefined();
            }));
        exports.SetProperty("ui", uiObj);
    }
}