#include "TextApi.h"
#include <Validators.h>

namespace TextApi {

Napi::Value TextApi::CreateText(const Napi::CallbackInfo& info)
{
  std::array<double, 4> argColor;

  auto argPosX = NapiHelper::ExtractDouble(info[0], "posX");
  auto argPosY = NapiHelper::ExtractDouble(info[1], "posY");

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto argString =
    converter.from_bytes(NapiHelper::ExtractString(info[2], "string"));

  auto color = NapiHelper::ExtractArray(info[3], "color");
  for (int i = 0; i < 4; i++) {
    std::string comment = fmt::format("color[{}]", i);
    argColor[i] = NapiHelper::ExtractDouble(color.Get(i), comment.data());
  }

  std::wstring fontName;

  if (!info[4].IsString() ||
      !ValidateFilename(static_cast<std::string>(info[4].As<Napi::String>()),
                        false)) {
    fontName = L"Tavern";
  } else {
    fontName = converter.from_bytes(
      static_cast<std::string>(info[4].As<Napi::String>()));
  }

  return Napi::Number::New(info.Env(),
                           TextsCollection::GetSingleton().CreateText(
                             argPosX, argPosY, argString, argColor, fontName));
}

Napi::Value TextApi::DestroyText(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  TextsCollection::GetSingleton().DestroyText(textId);
  return info.Env().Undefined();
}

Napi::Value TextApi::DestroyAllTexts(const Napi::CallbackInfo& info)
{
  TextsCollection::GetSingleton().DestroyAllTexts();
  return info.Env().Undefined();
}

Napi::Value SetTextsVisibility(const Napi::CallbackInfo& info)
{
  TextsVisibility::Ref() = TextsVisibility::FromString(
    NapiHelper::ExtractString(info[0], "visibility"));
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextPos(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto argPosX = NapiHelper::ExtractDouble(info[1], "posX");
  auto argPosY = NapiHelper::ExtractDouble(info[2], "posY");

  TextsCollection::GetSingleton().SetTextPos(textId, argPosX, argPosY);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextString(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto text = NapiHelper::ExtractString(info[1], "text");

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto argString = converter.from_bytes(text);
  auto moveArgString = std::move(argString);

  TextsCollection::GetSingleton().SetTextString(textId, moveArgString);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextColor(const Napi::CallbackInfo& info)
{
  std::array<double, 4> argColor;

  auto textId = NapiHelper::ExtractInt32(info[0], "textId");

  auto color = NapiHelper::ExtractArray(info[1], "color");
  for (int i = 0; i < 4; i++) {
    std::string comment = fmt::format("color[{}]", i);
    argColor[i] = NapiHelper::ExtractDouble(color.Get(i), comment.data());
  }

  auto moveArgColor = std::move(argColor);

  TextsCollection::GetSingleton().SetTextColor(textId, moveArgColor);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextSize(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");

  auto size = NapiHelper::ExtractFloat(info[1], "size");

  TextsCollection::GetSingleton().SetTextSize(textId, size);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextRotation(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");

  auto rot = NapiHelper::ExtractFloat(info[1], "rot");

  TextsCollection::GetSingleton().SetTextRotation(textId, rot);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextFont(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto name = NapiHelper::ExtractString(info[1], "name");

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  auto font = converter.from_bytes(name);

  TextsCollection::GetSingleton().SetTextFont(textId, font);

  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextDepth(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");

  auto depth = NapiHelper::ExtractInt32(info[1], "depth");

  TextsCollection::GetSingleton().SetTextDepth(textId, depth);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextEffect(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");

  auto effect = NapiHelper::ExtractInt32(info[1], "effect");

  TextsCollection::GetSingleton().SetTextEffect(textId, effect);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextOrigin(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto originArray = NapiHelper::ExtractArray(info[1], "origin");

  std::array<double, 2> argOrigin;

  for (int i = 0; i < 2; i++) {
    std::string comment = fmt::format("origin[{}]", i);
    argOrigin[i] =
      NapiHelper::ExtractDouble(originArray.Get(i), comment.data());
  }

  auto moveArgOrigin = std::move(argOrigin);

  TextsCollection::GetSingleton().SetTextOrigin(textId, moveArgOrigin);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextRefr(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto refrFormId = NapiHelper::ExtractUInt32(info[1], "refrFormId");

  TextsCollection::GetSingleton().SetTextRefr(textId, refrFormId);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextRefrNode(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto nodeName = NapiHelper::ExtractString(info[1], "nodeName");

  TextsCollection::GetSingleton().SetTextRefrNode(textId, nodeName);
  return info.Env().Undefined();
}

Napi::Value TextApi::SetTextRefrOffset(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto offsetArray = NapiHelper::ExtractArray(info[1], "offset");

  std::array<double, 3> argOffset;
  for (int i = 0; i < 3; i++) {
    std::string comment = fmt::format("offset[{}]", i);
    argOffset[i] =
      NapiHelper::ExtractDouble(offsetArray.Get(i), comment.data());
  }

  TextsCollection::GetSingleton().SetTextRefrOffset(textId, argOffset);
  return info.Env().Undefined();
}

Napi::Value TextApi::GetTextPos(const Napi::CallbackInfo& info)
{
  auto& postions = TextsCollection::GetSingleton().GetTextPos(
    NapiHelper::ExtractInt32(info[0], "textId"));
  auto jsArray = Napi::Array::New(info.Env(), 2);

  jsArray.Set(static_cast<uint32_t>(0),
              Napi::Number::New(info.Env(), postions.first));
  jsArray.Set(static_cast<uint32_t>(1),
              Napi::Number::New(info.Env(), postions.second));

  return jsArray;
}

TextsVisibility::Value GetTextsVisibility()
{
  return TextsVisibility::Ref();
}

Napi::Value GetTextsVisibilityJS(const Napi::CallbackInfo& info)
{
  return Napi::String::New(info.Env(), ToString(GetTextsVisibility()));
}

Napi::Value TextApi::GetTextString(const Napi::CallbackInfo& info)
{
  const auto& str = TextsCollection::GetSingleton().GetTextString(
    NapiHelper::ExtractInt32(info[0], "textId"));

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return Napi::String::New(info.Env(), converter.to_bytes(str));
}

Napi::Value TextApi::GetTextColor(const Napi::CallbackInfo& info)
{
  const auto& argArray = TextsCollection::GetSingleton().GetTextColor(
    NapiHelper::ExtractInt32(info[0], "textId"));
  auto jsArray = Napi::Array::New(info.Env(), 4);

  for (int i = 0; i < 4; i++) {
    jsArray.Set(i, Napi::Number::New(info.Env(), argArray.at(i)));
  }

  return jsArray;
}

Napi::Value TextApi::GetTextSize(const Napi::CallbackInfo& info)
{
  const auto& size = TextsCollection::GetSingleton().GetTextSize(
    NapiHelper::ExtractInt32(info[0], "textId"));

  return Napi::Number::New(info.Env(), size);
}

Napi::Value TextApi::GetTextRotation(const Napi::CallbackInfo& info)
{
  const auto& rot = TextsCollection::GetSingleton().GetTextRotation(
    NapiHelper::ExtractInt32(info[0], "textId"));

  return Napi::Number::New(info.Env(), rot);
}

Napi::Value TextApi::GetTextFont(const Napi::CallbackInfo& info)
{
  const auto& font = TextsCollection::GetSingleton().GetTextFont(
    NapiHelper::ExtractInt32(info[0], "textId"));

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return Napi::String::New(info.Env(), converter.to_bytes(font));
}

Napi::Value TextApi::GetTextDepth(const Napi::CallbackInfo& info)
{
  const auto& depth = TextsCollection::GetSingleton().GetTextDepth(
    NapiHelper::ExtractInt32(info[0], "textId"));

  return Napi::Number::New(info.Env(), depth);
}

Napi::Value TextApi::GetTextEffect(const Napi::CallbackInfo& info)
{
  const auto& effect = TextsCollection::GetSingleton().GetTextEffect(
    NapiHelper::ExtractInt32(info[0], "textId"));

  return Napi::Number::New(info.Env(), effect);
}

Napi::Value TextApi::GetTextOrigin(const Napi::CallbackInfo& info)
{
  auto argArray = TextsCollection::GetSingleton().GetTextColor(
    NapiHelper::ExtractInt32(info[0], "textId"));
  auto jsArray = Napi::Array::New(info.Env(), 2);

  for (int i = 0; i < 2; i++) {
    jsArray.Set(i, argArray.at(i));
  }

  return jsArray;
}

Napi::Value TextApi::GetTextRefr(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  auto refrFormId = TextsCollection::GetSingleton().GetTextRefr(textId);
  return Napi::Number::New(info.Env(), refrFormId);
}

Napi::Value TextApi::GetTextRefrNode(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  const auto& nodeName =
    TextsCollection::GetSingleton().GetTextRefrNode(textId);
  return Napi::String::New(info.Env(), nodeName);
}

Napi::Value TextApi::GetTextRefrOffset(const Napi::CallbackInfo& info)
{
  auto textId = NapiHelper::ExtractInt32(info[0], "textId");
  const auto& offset =
    TextsCollection::GetSingleton().GetTextRefrOffset(textId);
  auto jsArray = Napi::Array::New(info.Env(), 3);
  for (int i = 0; i < 3; i++) {
    jsArray.Set(i, Napi::Number::New(info.Env(), offset[i]));
  }
  return jsArray;
}

Napi::Value TextApi::GetNumCreatedTexts(const Napi::CallbackInfo& info)
{
  return Napi::Number::New(
    info.Env(), TextsCollection::GetSingleton().GetNumCreatedTexts());
}

namespace {
float g_screenWidth = 0.f;
float g_screenHeight = 0.f;

NiAVObject* ResolveNode(TESObjectREFR* obj, const std::string& nodeName)
{
  if (!obj) {
    return nullptr;
  }

  NiAVObject* result = obj->GetNiNode();

  // special-case for the player, switch between first/third-person
  if (obj->formID == 0x14) {
    PlayerCharacter* player = (PlayerCharacter*)obj;
    if (player->loadedState) {
      result = player->loadedState->node;
    }
  }

  // name lookup
  if (!nodeName.empty() && result) {
    BSFixedString bsName(nodeName.c_str());
    result = result->GetObjectByName(&bsName.data);
  }

  return result;
}
} // namespace

void OnUpdate()
{
  auto& texts = TextsCollection::GetSingleton().GetCreatedTexts();

  // Find any refr-attached text to know whether we need camera/screen setup
  bool anyAttached = false;
  for (auto& [id, text] : texts) {
    if (text.refrFormId != 0) {
      anyAttached = true;
      break;
    }
  }
  if (!anyAttached) {
    return;
  }

  // Resolve camera once
  auto* camera = RE::PlayerCamera::GetSingleton();
  if (!camera || !camera->cameraRoot) {
    return;
  }
  RE::NiCamera* niCamera = nullptr;
  for (uint16_t i = 0; i < camera->cameraRoot->children.size(); ++i) {
    auto* child = camera->cameraRoot->children[i].get();
    if (child) {
      niCamera = netimmerse_cast<RE::NiCamera*>(child);
      if (niCamera) {
        break;
      }
    }
  }
  if (!niCamera) {
    return;
  }

  // Read screen resolution once
  if (g_screenWidth == 0.f) {
    auto* settings = RE::INISettingCollection::GetSingleton();
    auto* wSetting =
      settings ? settings->GetSetting("iSize W:Display") : nullptr;
    auto* hSetting =
      settings ? settings->GetSetting("iSize H:Display") : nullptr;
    g_screenWidth =
      wSetting ? static_cast<float>(wSetting->GetSInt()) : 1920.f;
    g_screenHeight =
      hSetting ? static_cast<float>(hSetting->GetSInt()) : 1080.f;
  }

  for (auto& [id, text] : texts) {
    if (text.refrFormId == 0) {
      continue;
    }

    auto* refr = RE::TESForm::LookupByID<RE::TESObjectREFR>(text.refrFormId);
    if (!refr) {
      continue;
    }

    NiPoint3 worldPos;
    if (text.refrNodeName.empty()) {
      worldPos = refr->GetPosition();
    } else {
      NiAVObject* node = ResolveNode(refr, text.refrNodeName);
      if (!node) {
        continue;
      }
      worldPos = node->m_worldTransform.pos;
    }
    worldPos.x += static_cast<float>(text.refrOffset[0]);
    worldPos.y += static_cast<float>(text.refrOffset[1]);
    worldPos.z += static_cast<float>(text.refrOffset[2]);

    float outX, outY, outZ;
    RE::NiCamera::WorldPtToScreenPt3(niCamera->worldToCam, niCamera->port,
                                     worldPos, outX, outY, outZ, 1.f);

    // Only update position when the point is in front of the camera
    if (outZ <= 0.f) {
      continue;
    }

    // Match TypeScript: x = round(outX * width), y = round((1 - outY) *
    // height)
    text.x = std::round(outX * g_screenWidth);
    text.y = std::round((1.f - outY) * g_screenHeight);
  }
}

void Register(Napi::Env env, Napi::Object& exports)
{
  exports.Set(
    "createText",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(CreateText)));

  exports.Set(
    "destroyText",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(DestroyText)));
  exports.Set(
    "destroyAllTexts",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(DestroyAllTexts)));

  exports.Set("setTextsVisibility",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(SetTextsVisibility)));
  exports.Set(
    "setTextPos",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextPos)));
  exports.Set(
    "setTextString",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextString)));
  exports.Set(
    "setTextColor",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextColor)));
  exports.Set(
    "setTextSize",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextSize)));
  exports.Set(
    "setTextRotation",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextRotation)));
  exports.Set(
    "setTextFont",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextFont)));
  exports.Set(
    "setTextDepth",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextDepth)));
  exports.Set(
    "setTextEffect",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextEffect)));
  exports.Set(
    "setTextOrigin",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextOrigin)));
  exports.Set(
    "setTextRefr",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextRefr)));
  exports.Set(
    "setTextRefrNode",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(SetTextRefrNode)));
  exports.Set("setTextRefrOffset",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(SetTextRefrOffset)));

  exports.Set("getTextsVisibility",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetTextsVisibilityJS)));
  exports.Set(
    "getTextPos",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextPos)));
  exports.Set(
    "getTextString",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextString)));
  exports.Set(
    "getTextColor",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextColor)));
  exports.Set(
    "getTextSize",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextSize)));
  exports.Set(
    "getTextRotation",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextRotation)));
  exports.Set(
    "getTextFont",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextFont)));
  exports.Set(
    "getTextDepth",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextDepth)));
  exports.Set(
    "getTextEffect",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextEffect)));
  exports.Set(
    "getTextOrigin",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextOrigin)));
  exports.Set(
    "getTextRefr",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextRefr)));
  exports.Set(
    "getTextRefrNode",
    Napi::Function::New(env, NapiHelper::WrapCppExceptions(GetTextRefrNode)));
  exports.Set("getTextRefrOffset",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetTextRefrOffset)));

  exports.Set("getNumCreatedTexts",
              Napi::Function::New(
                env, NapiHelper::WrapCppExceptions(GetNumCreatedTexts)));
}

}
