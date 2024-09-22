#include "CameraApi.h"
#include "NullPointerException.h"

Napi::Value CameraApi::WorldPointToScreenPoint(const Napi::CallbackInfo& info)
{
  auto camera = RE::PlayerCamera::GetSingleton();
  if (!camera)
    throw NullPointerException("camera");
  auto camRoot = camera->cameraRoot;
  if (!camRoot)
    throw NullPointerException("camNode");
  auto n = camRoot->children.size();

  RE::NiCamera* niCamera = nullptr;
  for (uint16_t i = 0; i < n; ++i) {
    auto niAvObject = camRoot->children[i];
    if (!niAvObject)
      continue;
    niCamera = netimmerse_cast<RE::NiCamera*>(niAvObject.get());
    if (niCamera)
      break;
  }
  if (!niCamera)
    throw NullPointerException("matrix");

  auto length = info.Length();

  auto res = Napi::Array::New(info.Env(), length);

  char argNameForExtract[5] = "argX";

  for (size_t i = 0; i < length; ++i) {

    size_t charSizeT = static_cast<size_t>('0') + i;
    if (charSizeT <= 255 && i <= 9) {
      argNameForExtract[4] = static_cast<char>(charSizeT);
    }

    RE::NiPoint3 pos = NapiHelper::ExtractNiPoint3(info[i], argNameForExtract.data());
    float outX, outY, outZ;
    RE::NiCamera::WorldPtToScreenPt3(niCamera->worldToCam, niCamera->port, pos,
                                     outX, outY, outZ, 1.f);

    auto jsPos = Napi::Array::New(info.Env(), 3);
    jsPos.Set(0, Napi::Number::New(info.Env(), outX));
    jsPos.Set(1, Napi::Number::New(info.Env(), outY));
    jsPos.Set(2, Napi::Number::New(info.Env(), outZ));
    res.Set(i, jsPos);
  }

  return res;
}
