#include "CameraApi.h"
#include "JsExtractPoint.h"
#include "NullPointerException.h"
#include <skse64/GameCamera.h>
#include <skse64/NiNodes.h>
#include <skse64/NiObjects.h>
#include <skse64/NiRTTI.h>

JsValue CameraApi::WorldPointToScreenPoint(const JsFunctionArguments& args)
{
  auto camera = PlayerCamera::GetSingleton();
  if (!camera)
    throw NullPointerException("camera");
  auto camNode = camera->cameraNode;
  if (!camNode)
    throw NullPointerException("camNode");
  size_t n = camNode->m_children.m_size;

  NiCamera* niCamera = nullptr;
  for (size_t i = 0; i < n; ++i) {
    auto niAvObject = camNode->m_children.m_data[i];
    if (!niAvObject)
      continue;
    niCamera = ni_cast(niAvObject, NiCamera);
    if (niCamera)
      break;
  }
  if (!niCamera)
    throw NullPointerException("matrix");

  _WorldPtToScreenPt3_Internal f = WorldPtToScreenPt3_Internal;

  auto res = JsValue::Array(args.GetSize() - 1);

  for (size_t i = 1; i < args.GetSize(); ++i) {

    std::array<float, 3> arrayPos = JsExtractPoint(args[i]);
    NiPoint3 pos{ arrayPos[0], arrayPos[1], arrayPos[2] };
    float outX, outY, outZ;
    f((float*)&niCamera->m_aafWorldToCam, &niCamera->m_kPort, &pos, &outX,
      &outY, &outZ, 1.f);

    auto jsPos = JsValue::Array(3);
    jsPos.SetProperty(0, outX);
    jsPos.SetProperty(1, outY);
    jsPos.SetProperty(2, outZ);
    res.SetProperty(JsValue::Int(i - 1), jsPos);
  }

  return res;
}