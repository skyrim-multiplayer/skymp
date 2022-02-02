#pragma once
#include "EventHandlerMisc.h"
#include "EventHandlerSKSE.h"
#include "EventHandlerScript.h"
#include "EventHandlerStory.h"

class EventManager
{
public:
  [[nodiscard]] static EventManager* GetSingleton()
  {
    static EventManager singleton;
    return &singleton;
  }

  JsValue Subscribe();
  JsValue Unsubscribe();

private:
  EventManager(const EventManager&) = delete;
  EventManager(EventManager&&) = delete;

  ~EventManager() = default;

  EventHandlerMisc* _handlerMisc;
  EventHandlerSKSE* _handlerSKSE;
  EventHandlerScript* _handlerScript;
  EventHandlerStory* _handlerStory;
};
