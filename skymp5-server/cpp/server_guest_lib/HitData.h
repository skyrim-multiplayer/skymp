#pragma once
#include "JsonUtils.h"
#include <simdjson.h>

struct HitData
{
  // Aggressor player's form id
  uint32_t aggressor = 0;
  bool isBashAttack = false;
  bool isHitBlocked = false;
  bool isPowerAttack = false;
  bool isSneakAttack = false;
  uint32_t projectile = 0;
  // Weapon's form id
  uint32_t source = 0;
  // Target player's form id
  uint32_t target = 0;

  static HitData FromJson(const simdjson::dom::element& data)
  {
    JsonPointer aggressor("aggressor"), target("target"),
      isBashAttack("isBashAttack"), isHitBlocked("isHitBlocked"),
      isPowerAttack("isPowerAttack"), isSneakAttack("isSneakAttack"),
      projectile("projectile"), source("source");

    HitData result;
    ReadEx(data, aggressor, &result.aggressor);
    ReadEx(data, target, &result.target);
    ReadEx(data, isBashAttack, &result.isBashAttack);
    ReadEx(data, isHitBlocked, &result.isHitBlocked);
    ReadEx(data, isPowerAttack, &result.isPowerAttack);
    ReadEx(data, isSneakAttack, &result.isSneakAttack);
    ReadEx(data, projectile, &result.projectile);
    ReadEx(data, source, &result.source);
    return result;
  }
};
