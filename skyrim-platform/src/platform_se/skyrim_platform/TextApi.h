#pragma once
#include "TextsCollection.h"

#include "NapiHelper.h"

namespace TextApi {
void Register(Napi::Env env, Napi::Object& exports);

Napi::Value CreateText(const Napi::CallbackInfo& info);

Napi::Value DestroyText(const Napi::CallbackInfo& info);
Napi::Value DestroyAllTexts(const Napi::CallbackInfo& info);

namespace TextsVisibility {
enum Value;
} // namespace TextsVisibility

Napi::Value SetTextsVisibility(const Napi::CallbackInfo& info);
Napi::Value SetTextPos(const Napi::CallbackInfo& info);
Napi::Value SetTextString(const Napi::CallbackInfo& info);
Napi::Value SetTextColor(const Napi::CallbackInfo& info);
Napi::Value SetTextSize(const Napi::CallbackInfo& info);
Napi::Value SetTextRotation(const Napi::CallbackInfo& info);
Napi::Value SetTextFont(const Napi::CallbackInfo& info);
Napi::Value SetTextDepth(const Napi::CallbackInfo& info);
Napi::Value SetTextEffect(const Napi::CallbackInfo& info);
Napi::Value SetTextOrigin(const Napi::CallbackInfo& info);

TextsVisibility::Value GetTextsVisibility();
Napi::Value GetTextsVisibilityJS(const Napi::CallbackInfo& info);
Napi::Value GetTextPos(const Napi::CallbackInfo& info);
Napi::Value GetTextString(const Napi::CallbackInfo& info);
Napi::Value GetTextColor(const Napi::CallbackInfo& info);
Napi::Value GetTextSize(const Napi::CallbackInfo& info);
Napi::Value GetTextRotation(const Napi::CallbackInfo& info);
Napi::Value GetTextFont(const Napi::CallbackInfo& info);
Napi::Value GetTextDepth(const Napi::CallbackInfo& info);
Napi::Value GetTextEffect(const Napi::CallbackInfo& info);
Napi::Value GetTextOrigin(const Napi::CallbackInfo& info);

Napi::Value GetNumCreatedTexts(const Napi::CallbackInfo& info);

namespace TextsVisibility {
enum Value
{
  kInheritBrowser,
  kOff,
  kOn,
};

inline Value& Ref()
{
  static Value g_value = Value::kInheritBrowser;
  return g_value;
}

inline std::string_view ToStringView(Value visibility)
{
  switch (visibility) {
    case TextsVisibility::kInheritBrowser:
      return "inheritBrowser";
    case TextsVisibility::kOff:
      return "off";
    case TextsVisibility::kOn:
      return "on";
    default:
      std::terminate(); // unhandled value
  }
}

inline std::string ToString(Value visibility)
{
  return std::string{ToStringView(visibility)};
}

inline Value FromString(std::string_view s)
{
  if (s == "inheritBrowser") {
    return kInheritBrowser;
  }
  if (s == "off") {
    return kOff;
  }
  if (s == "on") {
    return kOn;
  }
  throw std::runtime_error("TextsVisibility: invalid enum value");
}
} // namespace TextsVisibility
} // namespace TextApi
