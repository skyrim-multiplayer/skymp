#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

struct Tint
{
  static Tint FromJson(simdjson::dom::element& j);

  std::string texturePath;
  int32_t argb = 0;
  int32_t type = 0;
};

struct Appearance
{
  // TODO: port to archives
  static Appearance FromJson(const nlohmann::json& j);
  static Appearance FromJson(simdjson::dom::element& j);
  std::string ToJson() const;

  bool isFemale = false;
  uint32_t raceId = 0;
  float weight = 0.f;
  int32_t skinColor = 0;
  int32_t hairColor = 0;
  std::vector<uint32_t> headpartIds;
  uint32_t headTextureSetId = 0;
  std::vector<float> faceMorphs;
  std::vector<float> facePresets;
  std::vector<Tint> tints;
  std::string name;
};
