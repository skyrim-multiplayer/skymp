#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct AnimationData
{
  AnimationData();

  std::string animEventName;
  uint32_t numChanges;

  static AnimationData FromJson(const simdjson::dom::element& data);
};
