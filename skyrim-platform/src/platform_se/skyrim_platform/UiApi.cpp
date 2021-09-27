#include "UiApi.h"
#include "EventsApi.h"
#include "GameEventSinks.h"
#include <RE/UI.h>
#include <RE/MenuOpenCloseEvent.h>

extern TaskQueue g_taskQueue;

namespace UiApi {
    void onMenuOpenClose(const char* menuName, bool opening)
    {
      g_taskQueue.AddTask([=] {
        auto obj = JsValue::Object();

        obj.SetProperty("name", JsValue::String(menuName));
        obj.SetProperty("type", JsValue::String(opening ? "open" : "close"));

        EventsApi::SendEvent("menuOpenClose", { JsValue::Undefined(), obj });
      });
    }

    class MyEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
    {
    public:
      ~MyEventSink(){};
      RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* e,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override
      {
        onMenuOpenClose(e->menuName.c_str(), e->opening);

        return RE::BSEventNotifyControl::kContinue;
      };
    };

    void Register(JsValue& exports)
    {
        auto ui = RE::UI::GetSingleton();

        if (!ui)
            return;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(new MyEventSink);
    }
}