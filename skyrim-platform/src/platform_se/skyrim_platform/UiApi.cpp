#include "UiApi.h"
#include "EventsApi.h"
#include "GameEventSinks.h"
#include <RE/MenuOpenCloseEvent.h>
#include <RE/UI.h>

extern TaskQueue g_taskQueue;

namespace UiApi {
class MyEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
  ~MyEventSink(){};
  RE::BSEventNotifyControl ProcessEvent(
    const RE::MenuOpenCloseEvent* e,
    RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override
  {
    const char* menuName = e->menuName.c_str();

    if (e->opening) {
      EventsApi::SendMenuOpen(menuName);
    } else {
      EventsApi::SendMenuClose(menuName);
    }

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