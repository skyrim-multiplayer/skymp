#include "FLGame.h"

void RegisterGameApi(std::shared_ptr<PartOne> partOne)
{
  JsValue globalObject = JsValue::GlobalObject();
  JsValue game = JsValue::Object();

  globalObject.SetProperty("Game", game);
}
