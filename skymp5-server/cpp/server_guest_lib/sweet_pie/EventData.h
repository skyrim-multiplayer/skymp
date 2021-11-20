#include "Data.h"
#include "Effect.h"
#include "Player.h"

namespace sweetpie {
struct EventData : public Data
{
  Player* target = nullptr;
  Player* aggressor = nullptr;
  float damage = 0.f;
  const NiPoint3* movement = nullptr;
  const std::set<Effect>* effects = nullptr;
};
}
