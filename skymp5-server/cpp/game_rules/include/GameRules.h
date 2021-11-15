#pragma once
#include <cstdint>

template <class State>
class GameRules
{
public:
  // GameRules doesn't know details of these ids meaning
  using Player = uint64_t;

  // empty string for default spawn point (i.e. from server settings)
  using SpawnPoint = const char*;

  virtual void EverySecond(State& gameState) = 0;
  virtual bool AddPlayer(State& gameState, Player player) = 0;
  virtual void RemovePlayer(State& gameState, Player player) = 0;
  virtual bool Hit(State& gameState, Player aggressor, Player target) = 0;
  virtual SpawnPoint GetSpawnPoint(Player playerToSpawn) = 0;
  virtual void KillPlayer(State& gameState, Player killer, Player dead) = 0;

  virtual ~GameRules() = default;
};