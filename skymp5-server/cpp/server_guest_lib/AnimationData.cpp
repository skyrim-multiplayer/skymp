#include "AnimationData.h"

AnimationData::AnimationData()
  : animEventName("")
  , numChanges(0)
{
}

AnimationData AnimationData::FromJson(const simdjson::dom::element& data)
{
  JsonPointer animEventName("animEventName"), numChanges("numChanges");

  AnimationData result;
  ReadEx(data, animEventName, &result.animEventName);
  ReadEx(data, numChanges, &result.numChanges);
  return result;
}
