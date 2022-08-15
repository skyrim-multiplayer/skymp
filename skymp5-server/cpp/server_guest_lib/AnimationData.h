#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct AnimationData
{
  AnimationData();

  const char* animEventName;
  uint32_t numChanges;

  static AnimationData FromJson(const simdjson::dom::element& data)
  {
    JsonPointer animEventName("animEventName"), numChanges("numChanges");

    AnimationData result;
    ReadEx(data, animEventName, &result.animEventName);
    ReadEx(data, numChanges, &result.numChanges);
    return result;
  }
};
