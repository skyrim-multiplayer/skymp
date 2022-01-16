#pragma once
#include "JsEngine.h"
#include <RE/TESObjectREFR.h>

class SKSETaskInterface;

namespace EventsApi {
JsValue On(const JsFunctionArguments& args);
JsValue Once(const JsFunctionArguments& args);
JsValue SendIpcMessage(const JsFunctionArguments& args);

void SendEvent(const char* eventName, const std::vector<JsValue>& arguments);
void Clear();

// Exceptions will be pushed to g_taskQueue
void SendAnimationEventEnter(uint32_t selfId,
                             std::string& animEventName) noexcept;
void SendAnimationEventLeave(bool animationSucceeded) noexcept;
void SendPapyrusEventEnter(uint32_t selfId,
                           std::string& papyrusEventName) noexcept;
void SendPapyrusEventLeave() noexcept;

JsValue GetHooks();

typedef void (*IpcMessageCallback)(const uint8_t* data, uint32_t length,
                                   void* state);
uint32_t IpcSubscribe(const char* systemName, IpcMessageCallback callback,
                      void* state);
void IpcUnsubscribe(uint32_t subscriptionId);
void IpcSend(const char* systemName, const uint8_t* data, uint32_t length);

void SendConsoleMsgEvent(const char* msg);
void SendMenuOpen(const char* menuName);
void SendMenuClose(const char* menuName);
void SendButtonEvent(uint32_t device, uint32_t code,
                     std::string& userEventName, float value,
                     float heldDownSecs, bool isPressed, bool isUp,
                     bool isDown, bool isHeld, bool isRepeating);
void SendMouseMoveEvent(uint32_t device, uint32_t code,
                        std::string& userEventName, double inputX,
                        double inputY);
void SendThumbstickEvent(uint32_t device, uint32_t code,
                         std::string& userEventName, float inputX,
                         float inputY, bool isLeft, bool isRight);
void SendKinectEvent(uint32_t device, uint32_t code,
                     std::string& userEventName, std::string& heard);
void SendDeviceConnectEvent(uint32_t device, bool isConnected);

inline void Register(JsValue& exports)
{
  exports.SetProperty("on", JsValue::Function(On));
  exports.SetProperty("once", JsValue::Function(Once));
  exports.SetProperty("hooks", GetHooks());
  exports.SetProperty("sendIpcMessage", JsValue::Function(SendIpcMessage));
}
}
