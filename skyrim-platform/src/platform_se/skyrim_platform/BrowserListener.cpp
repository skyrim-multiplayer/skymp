#include "BrowserListener.h"
#include "EventsApi.h" // EventsApi::SendEvent
#include "InputConverter.h"
#include "MyInputListener.h"
#include "TPInputService.h"
#include "TPOverlayService.h"
#include "TPRenderSystemD3D11.h"
#include "TaskQueue.h"
#include "hooks/D3D11Hook.hpp"
#include "hooks/DInputHook.hpp"
#include "hooks/WindowsHook.hpp"
#include "ui/ProcessMessageListener.h"
#include <skse64/NiRenderer.h>

namespace {
class ProcessMessageListenerImpl : public ProcessMessageListener
{
public:
  ProcessMessageListenerImpl(const std::shared_ptr<TaskQueue>& onTick_)
    : onTick(onTick_)
  {
  }

  void OnProcessMessage(
    const std::string& name,
    const CefRefPtr<CefListValue>& arguments_) noexcept override
  {
    try {
      HandleMessage(name, arguments_);
    } catch (const std::exception&) {
      auto exception = std::current_exception();
      onTick->AddTask([exception = std::move(exception)] {
        std::rethrow_exception(exception);
      });
    }
  }

private:
  void HandleMessage(const std::string& name,
                     const CefRefPtr<CefListValue>& arguments_)
  {
    auto arguments = arguments_->Copy();
    onTick->AddTask([name, arguments] {
      auto length = static_cast<uint32_t>(arguments->GetSize());
      auto argumentsArray = JsValue::Array(length);
      for (uint32_t i = 0; i < length; ++i) {
        argumentsArray.SetProperty(static_cast<int>(i),
                                   CefValueToJsValue(arguments->GetValue(i)));
      }

      auto browserMessageEvent = JsValue::Object();
      browserMessageEvent.SetProperty("arguments", argumentsArray);
      EventsApi::SendEvent("browserMessage",
                           { JsValue::Undefined(), browserMessageEvent });
    });
  }

  static JsValue CefValueToJsValue(const CefRefPtr<CefValue>& cefValue)
  {
    switch (cefValue->GetType()) {
      case VTYPE_NULL:
        return JsValue::Null();
      case VTYPE_BOOL:
        return JsValue::Bool(cefValue->GetBool());
      case VTYPE_INT:
        return JsValue::Int(cefValue->GetInt());
      case VTYPE_DOUBLE:
        return JsValue::Double(cefValue->GetDouble());
      case VTYPE_STRING:
        return JsValue::String(cefValue->GetString());
      case VTYPE_DICTIONARY: {
        auto dict = cefValue->GetDictionary();
        auto result = JsValue::Object();
        CefDictionaryValue::KeyList keyList;
        dict->GetKeys(keyList);
        for (const std::string& key : keyList) {
          auto cefValue = dict->GetValue(key);
          auto jsValue = CefValueToJsValue(cefValue);
          result.SetProperty(key, jsValue);
        }
        return result;
      }
      case VTYPE_LIST: {
        auto list = cefValue->GetList();
        auto length = static_cast<int>(list->GetSize());
        auto result = JsValue::Array(length);
        for (int i = 0; i < length; ++i) {
          auto cefValue = list->GetValue(i);
          auto jsValue = CefValueToJsValue(cefValue);
          result.SetProperty(i, jsValue);
        }
        return result;
      }
      case VTYPE_BINARY:
      case VTYPE_INVALID:
        return JsValue::Undefined();
    }
    return JsValue::Undefined();
  }

  std::shared_ptr<TaskQueue> onTick;
};
}

struct BrowserListener::Impl
{
  std::shared_ptr<TaskQueue> onTick;
  std::shared_ptr<BrowserApi::State> browserApiState;
  std::shared_ptr<OverlayService> overlayService;
  std::shared_ptr<RenderSystemD3D11> renderSystem;
  std::shared_ptr<MyInputListener> myInputListener;
  std::shared_ptr<IInputConverter> inputConverter;
};

BrowserListener::BrowserListener(
  std::shared_ptr<BrowserApi::State> browserApiState)
{
  pImpl = std::make_shared<Impl>();
  pImpl->onTick = std::make_shared<TaskQueue>();
  pImpl->browserApiState = browserApiState;
}

void BrowserListener::Tick()
{
  pImpl->onTick->Update();
}

void BrowserListener::Update()
{
}

void BrowserListener::BeginMain()
{
  pImpl->inputConverter = std::make_shared<InputConverter>();
  pImpl->myInputListener = std::make_shared<MyInputListener>();

  CEFUtils::D3D11Hook::Install();
  CEFUtils::DInputHook::Install(pImpl->myInputListener);
  CEFUtils::WindowsHook::Install();

  CEFUtils::DInputHook::Get().SetToggleKeys({ VK_F6 });
  CEFUtils::DInputHook::Get().SetEnabled(true);

  auto onProcessMessage =
    std::make_shared<ProcessMessageListenerImpl>(pImpl->onTick);

  pImpl->overlayService = std::make_shared<OverlayService>(onProcessMessage);
  pImpl->myInputListener->Init(pImpl->overlayService, pImpl->inputConverter);
  pImpl->browserApiState->overlayService = pImpl->overlayService;

  pImpl->renderSystem =
    std::make_shared<RenderSystemD3D11>(*pImpl->overlayService);
  pImpl->renderSystem->m_pSwapChain = reinterpret_cast<IDXGISwapChain*>(
    BSRenderManager::GetSingleton()->swapChain);
}

void BrowserListener::EndMain()
{
  pImpl->renderSystem.reset();
  pImpl->overlayService.reset();
}