#pragma once
#include "TextsCollection.h"

#include "NapiHelper.h"

namespace TextApi {
void Register(Napi::Env env, Napi::Object& exports);

Napi::Value CreateText(const Napi::CallbackInfo &info);

Napi::Value DestroyText(const Napi::CallbackInfo &info);
Napi::Value DestroyAllTexts(const Napi::CallbackInfo &info);

Napi::Value SetTextPos(const Napi::CallbackInfo &info);
Napi::Value SetTextString(const Napi::CallbackInfo &info);
Napi::Value SetTextColor(const Napi::CallbackInfo &info);
Napi::Value SetTextSize(const Napi::CallbackInfo &info);
Napi::Value SetTextRotation(const Napi::CallbackInfo &info);
Napi::Value SetTextFont(const Napi::CallbackInfo &info);
Napi::Value SetTextDepth(const Napi::CallbackInfo &info);
Napi::Value SetTextEffect(const Napi::CallbackInfo &info);
Napi::Value SetTextOrigin(const Napi::CallbackInfo &info);

Napi::Value GetTextPos(const Napi::CallbackInfo &info);
Napi::Value GetTextString(const Napi::CallbackInfo &info);
Napi::Value GetTextColor(const Napi::CallbackInfo &info);
Napi::Value GetTextSize(const Napi::CallbackInfo &info);
Napi::Value GetTextRotation(const Napi::CallbackInfo &info);
Napi::Value GetTextFont(const Napi::CallbackInfo &info);
Napi::Value GetTextDepth(const Napi::CallbackInfo &info);
Napi::Value GetTextEffect(const Napi::CallbackInfo &info);
Napi::Value GetTextOrigin(const Napi::CallbackInfo &info);

Napi::Value GetNumCreatedTexts(const Napi::CallbackInfo &info);
}
