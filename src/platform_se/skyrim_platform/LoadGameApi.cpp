#include "LoadGameApi.h"
#include "JsExtractPoint.h"
#include "LoadGame.h"
#include <memory>

JsValue LoadGameApi::LoadGame(const JsFunctionArguments& args)
{
  std::array<float, 3> pos = JsExtractPoint(args[1]),
                       angle = JsExtractPoint(args[2]);
  uint32_t cellOrWorld = (uint32_t)(double)args[3];

  LoadGame::Run(pos, angle, cellOrWorld);
  return JsValue::Undefined();
}