#pragma once
#include "TextsCollection.h"
#include "JsEngine.h"
#include "ui/TextToDraw.h"

namespace TextApi 
{
  void Register(JsValue& exports);

  JsValue CreateText(const JsFunctionArguments& args);

  JsValue DestroyText(const JsFunctionArguments& args);

  JsValue SetTextPos(const JsFunctionArguments& args);

  JsValue SetTextString(const JsFunctionArguments& args);

  JsValue SetTextColor(const JsFunctionArguments& args);
  
  JsValue DestroyAllTexts();
	
  JsValue GetTextPos(const JsFunctionArguments& args);

  JsValue GetTextString(const JsFunctionArguments& args);

  JsValue GetTextColor(const JsFunctionArguments& args);

  JsValue GetCreatedTexts(const JsFunctionArguments& args);

  JsValue GetTextCount(const JsFunctionArguments& args);
}