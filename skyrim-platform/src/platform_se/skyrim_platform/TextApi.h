#pragma once
#include "TextsCollection.h"

namespace TextApi {
void Register(JsValue& exports);

JsValue CreateText(const JsFunctionArguments& args);

JsValue DestroyText(const JsFunctionArguments& args);

JsValue SetTextPos(const JsFunctionArguments& args);

JsValue SetTextString(const JsFunctionArguments& args);

JsValue SetTextColor_(const JsFunctionArguments& args);

JsValue DestroyAllTexts(const JsFunctionArguments& args);

JsValue GetTextPos(const JsFunctionArguments& args);

JsValue GetTextString(const JsFunctionArguments& args);

JsValue GetTextColor(const JsFunctionArguments& args);

JsValue GetNumCreatedTexts(const JsFunctionArguments& args);
}
