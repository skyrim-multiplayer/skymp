#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct AnimationData
{
  std::string animEventName = "";
  uint32_t numChanges = 0;

  static AnimationData FromJson(const simdjson::dom::element& data)
  {
    JsonPointer animEventName("animationName"), numChanges("numChanges");

    AnimationData result;
    ReadEx(data, animEventName, &result.animEventName);
    ReadEx(data, numChanges, &result.numChanges);
    return result;
  }
};
