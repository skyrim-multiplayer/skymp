#pragma once
#include "Data.h"
#include "Effect.h"
#include "Player.h"

namespace sweetpie {
enum EventType : uint16_t
{
  OnHit,
  OnKill,
  end
};
struct EventData : public Data
{
  EventType type = end;
  Player target;
  Player aggressor;
  float damageMod = 0.f;
  std::set<Effect> effectsMod;
};
}
