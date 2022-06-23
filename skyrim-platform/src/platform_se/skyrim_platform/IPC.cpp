#include "IPC.h"
#include "EventHandler.h"
#include "JsUtils.h"

struct Share
{
  std::recursive_mutex m;
  std::vector<std::shared_ptr<IPC::CallbackData>> ipcCallbacks;
} g_ipcShare;

uint32_t IPC::Subscribe(const char* systemName, IPC::MessageCallback callback,
                        void* state)
{
  // Maybe they decide calling IpcSubscribe from multiple threads...
  std::lock_guard l(g_ipcShare.m);

  auto cbData = std::make_shared<CallbackData>(systemName, callback, state);
  g_ipcShare.ipcCallbacks.emplace_back(cbData);

  return g_ipcShare.ipcCallbacks.size() - 1;
}

void IPC::Unsubscribe(uint32_t subscriptionId)
{
  std::lock_guard l(g_ipcShare.m);
  g_ipcShare.ipcCallbacks.erase(
    std::next(g_ipcShare.ipcCallbacks.begin(), subscriptionId));
}

void IPC::Call(const std::string& systemName, const uint8_t* data,
               uint32_t length)
{
  std::vector<std::shared_ptr<CallbackData>> callbacks;

  std::lock_guard l(g_ipcShare.m);
  std::copy_if(g_ipcShare.ipcCallbacks.begin(), g_ipcShare.ipcCallbacks.end(),
               std::back_inserter(callbacks),
               [systemName](const std::shared_ptr<CallbackData> cbData) {
                 return cbData->systemName == systemName;
               });

  // Want to call callbacks with g_ipcShare.m unlocked
  for (auto& callbackData : callbacks) {
    if (callbackData->callback) {
      callbackData->callback(data, length, callbackData->state);
    }
  }
}

void IPC::Send(const char* systemName, const uint8_t* data, uint32_t length)
{
  auto obj = JsValue::Object();
  AddObjProperty(&obj, "sourceSystemName", systemName);
  AddObjProperty(&obj, "message", data, length);

  EventHandler::SendEventOnTick("ipcMessage", obj);
}
