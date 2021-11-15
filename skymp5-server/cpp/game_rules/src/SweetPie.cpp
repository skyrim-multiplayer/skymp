#include "SweetPie.h"

void SweetPie::EverySecond(SweetPieState& gameState)
{
}

bool SweetPie::AddPlayer(SweetPieState& gameState, Player player)
{
  return true;
}

void SweetPie::RemovePlayer(SweetPieState& gameState, Player player)
{
}

bool SweetPie::Hit(SweetPieState& gameState, Player aggressor, Player target)
{
  return true;
}

SweetPie::SpawnPoint SweetPie::GetSpawnPoint(Player playerToSpawn)
{
  return "";
}

void SweetPie::KillPlayer(SweetPieState& gameState, Player killer, Player dead)
{
}