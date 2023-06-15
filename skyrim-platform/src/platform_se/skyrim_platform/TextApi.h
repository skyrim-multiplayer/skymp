#pragma once
#include "TextsCollection.h"

namespace TextApi {
void Register(JsValue& exports);

JsValue CreateText(const JsFunctionArguments& args);

JsValue DestroyText(const JsFunctionArguments& args);
JsValue DestroyAllTexts(const JsFunctionArguments& args);

JsValue SetTextPos(const JsFunctionArguments& args);
JsValue SetTextString(const JsFunctionArguments& args);
JsValue SetTextColor(const JsFunctionArguments& args);
JsValue SetTextSize(const JsFunctionArguments& args);
JsValue SetTextRotation(const JsFunctionArguments& args);
JsValue SetTextFont(const JsFunctionArguments& args);
JsValue SetTextDepth(const JsFunctionArguments& args);
JsValue SetTextEffect(const JsFunctionArguments& args);
JsValue SetTextOrigin(const JsFunctionArguments& args);

JsValue GetTextPos(const JsFunctionArguments& args);
JsValue GetTextString(const JsFunctionArguments& args);
JsValue GetTextColor(const JsFunctionArguments& args);
JsValue GetTextSize(const JsFunctionArguments& args);
JsValue GetTextRotation(const JsFunctionArguments& args);
JsValue GetTextFont(const JsFunctionArguments& args);
JsValue GetTextDepth(const JsFunctionArguments& args);
JsValue GetTextEffect(const JsFunctionArguments& args);
JsValue GetTextOrigin(const JsFunctionArguments& args);

JsValue GetNumCreatedTexts(const JsFunctionArguments& args);
}
