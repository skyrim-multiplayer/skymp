#include "UiApi.h"
#include "EventsApi.h"
#include "GameEventSinks.h"
#include <RE/UI.h>

extern TaskQueue g_taskQueue;

namespace RE {
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
    //>>>
}

void onMenuOpenClose(const char * menuName, bool opening) {
    g_taskQueue.AddTask([=] {
        auto obj = JsValue::Object();

        obj.SetProperty("name", JsValue::String(menuName));
        obj.SetProperty("type", JsValue::String( opening ? "open" : "close"));

        EventsApi::SendEvent("menuOpenClose", { JsValue::Undefined(), obj });
    });
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
        auto ui = RE::UI::GetSingleton();

        if (!ui)
            return;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(new MyEventSink);
    }
}