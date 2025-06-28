#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <simdjson.h>
#include <string>

struct Tint
{
  // TODO: get rid of these methods
  static Tint FromJson(simdjson::dom::element& element);

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("texturePath", texturePath)
      .Serialize("argb", argb)
      .Serialize("type", type);
  }

  std::string texturePath;
  int32_t argb = 0;
  int32_t type = 0;

  friend bool operator==(const Tint& lhs, const Tint& rhs)
  {
    return lhs.texturePath == rhs.texturePath && lhs.argb == rhs.argb &&
      lhs.type == rhs.type;
  }

  friend bool operator!=(const Tint& lhs, const Tint& rhs)
  {
    return !(lhs == rhs);
  }
};

struct Appearance
{
  // TODO: get rid of these methods
  static Appearance FromJson(const nlohmann::json& j);
  static Appearance FromJson(simdjson::dom::element& j);
  std::string ToJson() const;

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("isFemale", isFemale)
      .Serialize("raceId", raceId)
      .Serialize("weight", weight)
      .Serialize("skinColor", skinColor)
      .Serialize("hairColor", hairColor)
      .Serialize("headpartIds", headpartIds)
      .Serialize("headTextureSetId", headTextureSetId)
      .Serialize("options", options)
      .Serialize("presets", presets)
      .Serialize("tints", tints)
      .Serialize("name", name);
  }

  bool isFemale = false;
  uint32_t raceId = 0;
  float weight = 0.f;
  int32_t skinColor = 0;
  int32_t hairColor = 0;
  std::vector<uint32_t> headpartIds;
  uint32_t headTextureSetId = 0;
  std::vector<float> options; // faceMorphs
  std::vector<float> presets; // facePresets
  std::vector<Tint> tints;
  std::string name;

  friend bool operator==(const Appearance& lhs, const Appearance& rhs)
  {
    return lhs.isFemale == rhs.isFemale && lhs.raceId == rhs.raceId &&
      lhs.weight == rhs.weight && lhs.skinColor == rhs.skinColor &&
      lhs.hairColor == rhs.hairColor && lhs.headpartIds == rhs.headpartIds &&
      lhs.headTextureSetId == rhs.headTextureSetId &&
      lhs.options == rhs.options && lhs.presets == rhs.presets &&
      lhs.tints == rhs.tints && lhs.name == rhs.name;
  }

  friend bool operator!=(const Appearance& lhs, const Appearance& rhs)
  {
    return !(lhs == rhs);
  }
};
