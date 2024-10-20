#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

enum class SpellType
{
  Left,
  Right,
  Voise,
  Instant,
};

struct SpellCastData
{
  uint32_t caster = 0;
  uint32_t target = 0;
  uint32_t spell = 0;

  bool isDualCasting = false;
  bool isInterruptCast = false;

  SpellType castingSource = SpellType::Left;

  static SpellCastData FromJson(const simdjson::dom::element& data)
  {
    const JsonPointer caster("caster"), target("target"), spell("spell"),
      interruptCast("interruptCast"), isDualCasting("isDualCasting"),
      castingSource("castingSource");

    SpellCastData result;
    ReadEx(data, caster, &result.caster);
    ReadEx(data, target, &result.target);
    ReadEx(data, spell, &result.spell);
    ReadEx(data, interruptCast, &result.isInterruptCast);
    ReadEx(data, isDualCasting, &result.isDualCasting);

    uint32_t cSource = 0;
    ReadEx(data, castingSource, &cSource);
    result.castingSource = static_cast<SpellType>(cSource);

    return result;
  }
};
