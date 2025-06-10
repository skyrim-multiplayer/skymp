#pragma once
#include <simdjson.h>
#include <nlohmann/json_fwd.hpp>

struct VoiceChat
{
  // TODO: get rid in favor of Serialize
  nlohmann::json ToJson() const;
  static VoiceChat FromJson(const simdjson::dom::element& element);

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("isTalking", isTalking);
  }

  bool isTalking = false;
};
