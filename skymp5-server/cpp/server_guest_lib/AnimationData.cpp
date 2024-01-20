#include "AnimationData.h"

AnimationData::AnimationData()
  : animEventName("")
  , numChanges(0)
{
}

AnimationData AnimationData::FromJson(const simdjson::dom::element& data)
{
  JsonPointer animEventName("animEventName"), numChanges("numChanges");

  const char* animEventNameStr = "";

  AnimationData result;
  ReadEx(data, animEventName, &animEventName);
  ReadEx(data, numChanges, &result.numChanges);

  result.animEventName = animEventNameStr;
  return result;
}
