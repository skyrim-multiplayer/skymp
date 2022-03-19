#include "CameraApi.h"
#include "JsExtractPoint.h"
#include "NullPointerException.h"

JsValue CameraApi::WorldPointToScreenPoint(const JsFunctionArguments& args)
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

  auto res = JsValue::Array(args.GetSize() - 1);

  for (size_t i = 1; i < args.GetSize(); ++i) {

    std::array<float, 3> arrayPos = JsExtractPoint(args[i]);
    RE::NiPoint3 pos{ arrayPos[0], arrayPos[1], arrayPos[2] };
    float outX, outY, outZ;
    RE::NiCamera::WorldPtToScreenPt3(niCamera->worldToCam, niCamera->port, pos,
                                     outX, outY, outZ, 1.f);

    auto jsPos = JsValue::Array(3);
    jsPos.SetProperty(0, outX);
    jsPos.SetProperty(1, outY);
    jsPos.SetProperty(2, outZ);
    res.SetProperty(JsValue::Int(i - 1), jsPos);
  }

  return res;
}
