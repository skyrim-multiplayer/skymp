#include "Appearance.h"
#include "JsonUtils.h"

Tint Tint::FromJson(simdjson::dom::element& j)
{
  static const JsonPointer argb("argb"), type("type"),
    texturePath("texturePath");

  Tint res;
  ReadEx(j, argb, &res.argb);
  ReadEx(j, type, &res.type);

  const char* texturePathCstr = "";
  ReadEx(j, texturePath, &texturePathCstr);
  res.texturePath = texturePathCstr;

  return res;
}

Appearance Appearance::FromJson(const nlohmann::json& j)
{
  simdjson::dom::parser p;
  simdjson::dom::element parsed = p.parse(j.dump());
  return FromJson(parsed);
}

Appearance Appearance::FromJson(simdjson::dom::element& j)
{
  static const JsonPointer isFemale("isFemale"), raceId("raceId"),
    weight("weight"), skinColor("skinColor"), hairColor("hairColor"),
    headpartIds("headpartIds"), headTextureSetId("headTextureSetId"),
    options("options"), presets("presets"), name("name"), tints("tints");

  Appearance res;
  ReadEx(j, isFemale, &res.isFemale);
  ReadEx(j, raceId, &res.raceId);
  ReadEx(j, weight, &res.weight);
  ReadEx(j, skinColor, &res.skinColor);
  ReadEx(j, hairColor, &res.hairColor);
  ReadVector(j, headpartIds, &res.headpartIds);
  ReadEx(j, headTextureSetId, &res.headTextureSetId);
  ReadVector(j, options, &res.faceMorphs);
  ReadVector(j, presets, &res.facePresets);

  const char* myName;
  try {
    Read(j, name, &myName);
  } catch (std::exception&) {
    myName = "";
  }
  res.name = myName;

  simdjson::dom::element jTints;
  ReadEx(j, tints, &jTints);

  res.tints.reserve(30);
  auto jTintsArr = jTints.operator simdjson::dom::array();
  for (simdjson::dom::element el : jTintsArr) {
    res.tints.push_back(Tint::FromJson(el));
  }

  return res;
}

std::string Appearance::ToJson() const
{
  auto j = nlohmann::json{ { "isFemale", isFemale },
                           { "raceId", raceId },
                           { "weight", weight },
                           { "skinColor", skinColor },
                           { "hairColor", hairColor },
                           { "headpartIds", headpartIds },
                           { "headTextureSetId", headTextureSetId },
                           { "options", faceMorphs },
                           { "presets", facePresets },
                           { "name", name } };
  j["tints"] = nlohmann::json::array();
  for (auto& tint : tints) {
    j["tints"].push_back(nlohmann::json{ { "texturePath", tint.texturePath },
                                         { "argb", tint.argb },
                                         { "type", tint.type } });
  }
  return j.dump();
}
