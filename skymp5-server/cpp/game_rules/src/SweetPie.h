#pragma once
#include "GameRules.h"

struct SweetPieState
{
};

class SweetPie : public GameRules<SweetPieState>
{
public:
  void EverySecond(SweetPieState& gameState) override;

  bool AddPlayer(SweetPieState& gameState, Player player) override;

  void RemovePlayer(SweetPieState& gameState, Player player) override;

  bool Hit(SweetPieState& gameState, Player aggressor, Player target) override;

  SpawnPoint GetSpawnPoint(Player playerToSpawn) override;

  void KillPlayer(SweetPieState& gameState, Player killer,
                  Player dead) override;
};