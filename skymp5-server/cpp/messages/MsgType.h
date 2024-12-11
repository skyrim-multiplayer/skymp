#pragma once
#include <cstdint>

enum class MsgType : uint8_t
{
  Invalid = 0,
  CustomPacket = 1,
  UpdateMovement = 2,
  UpdateAnimation = 3,
  UpdateAppearance = 4,
  UpdateEquipment = 5,
  Activate = 6,
  UpdateProperty = 7,
  PutItem = 8,
  TakeItem = 9,
  FinishSpSnippet = 10,
  OnEquip = 11,
  ConsoleCommand = 12,
  CraftItem = 13,
  Host = 14,
  CustomEvent = 15,
  ChangeValues = 16,
  OnHit = 17,
  DeathStateContainer = 18,
  DropItem = 19,
  Teleport = 20,
  OpenContainer = 21,
  PlayerBowShot = 22,
  SpellCast = 23,
  UpdateAnimVariables = 24,

  // ex-strings
  DestroyActor = 100,

  Max
};
