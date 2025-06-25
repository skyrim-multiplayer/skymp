#include "AnimationData.h"

#include "archives/SimdJsonInputArchive.h"

AnimationData AnimationData::FromJson(const simdjson::dom::element& element)
{
  SimdJsonInputArchive ar(element);
  AnimationData res;
  res.Serialize(ar);
  return res;
}
