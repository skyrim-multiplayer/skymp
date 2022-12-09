#include "OverlayV8Handler.h"
#include "../core_library/Platform.hpp"
#include <algorithm>

namespace CEFUtils {
OverlayV8Handler::OverlayV8Handler(
  const CefRefPtr<CefBrowser>& apBrowser) noexcept
  : m_pBrowser(apBrowser)
{
}

bool OverlayV8Handler::Execute(const CefString& acName,
                               CefRefPtr<CefV8Value> apObject,
                               const CefV8ValueList& acArguments,
                               CefRefPtr<CefV8Value>& aReturnValue,
                               CefString& aException)
{
  TP_UNUSED(apObject);
  TP_UNUSED(aReturnValue);

  Dispatch(acName, acArguments, aException);

  return true;
}

void OverlayV8Handler::Dispatch(const CefString& acName,
                                const CefV8ValueList& acArguments,
                                CefString& aException) const noexcept
{
  auto pMessage = CefProcessMessage::Create("ui-event");

  auto pArguments = pMessage->GetArgumentList();
  auto pArgumentsList = CefListValue::Create();

  std::vector<CefRefPtr<CefV8Value>> stack;

  for (size_t i = 0; i < acArguments.size(); ++i) {
    pArgumentsList->SetValue(static_cast<int32_t>(i),
                             ConvertValue(acArguments[i], aException, stack));
  }

  if (aException.empty()) {
    pArguments->SetString(0, acName);
    pArguments->SetList(1, pArgumentsList);

    m_pBrowser->GetMainFrame()->SendProcessMessage(PID_BROWSER, pMessage);
  }
}

CefRefPtr<CefValue> OverlayV8Handler::ConvertValue(
  const CefRefPtr<CefV8Value>& acpValue, CefString& aException,
  std::vector<CefRefPtr<CefV8Value>>& stack)
{
  auto pValue = CefValue::Create();
  auto sameCount = std::count_if(stack.begin(), stack.end(),
                                 [&](const CefRefPtr<CefV8Value>& stackValue) {
                                   return stackValue->IsSame(acpValue);
                                 });

  if (sameCount > 0) {
    // Throwing exceptions crash for some reason so just write to string
    aException = "Serializing circular structure";
    return pValue;
  }

  if (acpValue->IsBool()) {
    pValue->SetBool(acpValue->GetBoolValue());
  } else if (acpValue->IsInt()) {
    pValue->SetInt(acpValue->GetIntValue());
  } else if (acpValue->IsDouble()) {
    pValue->SetDouble(acpValue->GetDoubleValue());
  } else if (acpValue->IsNull()) {
    pValue->SetNull();
  } else if (acpValue->IsString()) {
    pValue->SetString(acpValue->GetStringValue());
  } else if (acpValue->IsArray()) {
    auto pList = CefListValue::Create();

    stack.push_back(acpValue);
    for (int i = 0; i < acpValue->GetArrayLength(); ++i) {
      pList->SetValue(i,
                      ConvertValue(acpValue->GetValue(i), aException, stack));
    }
    stack.pop_back();

    pValue->SetList(pList);
  } else if (acpValue->IsObject()) {
    auto pDict = CefDictionaryValue::Create();

    std::vector<CefString> keys;
    acpValue->GetKeys(keys);

    stack.push_back(acpValue);
    for (const auto& key : keys) {
      pDict->SetValue(
        key, ConvertValue(acpValue->GetValue(key), aException, stack));
    }
    stack.pop_back();

    pValue->SetDictionary(pDict);
  }

  return pValue;
}
}
