#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct AnimationData
{
  const char* animEventName = "";
  uint32_t numChanges = 0;

  static AnimationData FromJson(const simdjson::dom::element& data)
  {
    JsonPointer animEventName("animEventName"), numChanges("numChanges");

    AnimationData result;
    ReadEx(data, animEventName, &result.animEventName);
    ReadEx(data, numChanges, &result.numChanges);
    return result;
  }
};
