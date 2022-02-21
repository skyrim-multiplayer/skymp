#include "TextApi.h"

namespace TextApi {

JsValue TextApi::CreateText(const JsFunctionArguments& args)
{
  std::array<double, 4> argColor;

  auto argPosX = static_cast<double>(args[1]);
  auto argPosY = static_cast<double>(args[2]);

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto argString = converter.from_bytes(static_cast<std::string>(args[3]));

  for (int i = 0; i < 4; i++) {
    argColor[i] = args[4].GetProperty(i);
  }

  return JsValue(TextsCollection::GetSingleton().CreateText(
    argPosX, argPosY, argString, argColor));
}

JsValue TextApi::DestroyText(const JsFunctionArguments& args)
{
  TextsCollection::GetSingleton().DestroyText(static_cast<int>(args[1]));
  return JsValue::Undefined();
}

JsValue TextApi::SetTextPos(const JsFunctionArguments& args)
{
  auto argPosX = static_cast<double>(args[2]);
  auto argPosY = static_cast<double>(args[3]);

  TextsCollection::GetSingleton().SetTextPos(static_cast<int>(args[1]),
                                             argPosX, argPosY);
  return JsValue::Undefined();
}

JsValue TextApi::SetTextString(const JsFunctionArguments& args)
{
  auto textId = static_cast<int>(args[1]);

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto argString = converter.from_bytes(static_cast<std::string>(args[2]));

  TextsCollection::GetSingleton().SetTextString(textId, std::move(argString));
  return JsValue::Undefined();
}

JsValue TextApi::SetTextColor_(const JsFunctionArguments& args)
{
  std::array<double, 4> argColor;

  for (int i = 0; i < 4; i++) {
    argColor[i] = args[2].GetProperty(i);
  }

  TextsCollection::GetSingleton().SetTextColor(static_cast<int>(args[1]),
                                               std::move(argColor));
  return JsValue::Undefined();
}

JsValue TextApi::DestroyAllTexts(const JsFunctionArguments&)
{
  TextsCollection::GetSingleton().DestroyAllTexts();
  return JsValue::Undefined();
}

JsValue TextApi::GetTextPos(const JsFunctionArguments& args)
{
  auto postions =
    TextsCollection::GetSingleton().GetTextPos(static_cast<int>(args[1]));
  auto jsArray = JsValue::Array(2);

  jsArray.SetProperty(0, postions.first);
  jsArray.SetProperty(1, postions.second);

  return jsArray;
}

JsValue TextApi::GetTextString(const JsFunctionArguments& args)
{
  const auto& str =
    TextsCollection::GetSingleton().GetTextString(static_cast<int>(args[1]));

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return JsValue(converter.to_bytes(str));
}

JsValue TextApi::GetTextColor(const JsFunctionArguments& args)
{
  auto argArray =
    TextsCollection::GetSingleton().GetTextColor(static_cast<int>(args[1]));
  auto jsArray = JsValue::Array(4);

  for (int i = 0; i < 4; i++) {
    jsArray.SetProperty(i, argArray.at(i));
  }

  return jsArray;
}

JsValue TextApi::GetNumCreatedTexts(const JsFunctionArguments& args)
{
  return JsValue(TextsCollection::GetSingleton().GetNumCreatedTexts());
}

void Register(JsValue& exports)
{
  exports.SetProperty("createText", JsValue::Function(CreateText));

  exports.SetProperty("destroyText", JsValue::Function(DestroyText));

  exports.SetProperty("setTextPos", JsValue::Function(SetTextPos));

  exports.SetProperty("setTextString", JsValue::Function(SetTextString));

  exports.SetProperty("setTextColor", JsValue::Function(SetTextColor_));

  exports.SetProperty("destroyAllTexts", JsValue::Function(DestroyAllTexts));

  exports.SetProperty("getTextPos", JsValue::Function(GetTextPos));

  exports.SetProperty("getTextString", JsValue::Function(GetTextString));

  exports.SetProperty("getTextColor", JsValue::Function(GetTextColor));

  exports.SetProperty("getNumCreatedTexts",
                      JsValue::Function(GetNumCreatedTexts));
}

}
