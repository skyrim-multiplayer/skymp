#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct AnimationData
{
  // TODO: get rid of FromJson method in favor of archives
  static AnimationData FromJson(const simdjson::dom::element& element);

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("animEventName", animEventName)
      .Serialize("numChanges", numChanges);
  }

  std::string animEventName;
  uint32_t numChanges = 0;
};
