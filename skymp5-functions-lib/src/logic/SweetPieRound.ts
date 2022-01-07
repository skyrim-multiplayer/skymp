import { PlayerController } from "./PlayerController";
import { SweetPieMap } from "./SweetPieMap";

export type SweetPieRound = {
  state: 'running' | 'warmup';
  players?: Map<number, { kills?: number }>;
  map?: SweetPieMap;
  hallPointName?: string;
  secondsPassed?: number;
}

export const getAvailableRound = (rounds: SweetPieRound[], player: number): SweetPieRound | undefined => {
  return rounds.find((x) => x.state !== 'running');
};

export const getPlayerCurrentRound = (rounds: SweetPieRound[], player: number): SweetPieRound | undefined => {
  return rounds.find((x) => x.players && x.players.has(player));
};

export const forceJoinRound = (controller: PlayerController, rounds: SweetPieRound[], round: SweetPieRound, player: number): void => {
  if (round.map) {
    controller.setSpawnPoint(player, round.map.safePointName);
    controller.teleport(player, round.map.safePointName);
    round.players = round.players || new Map;
    round.players.set(player, {});
  }
};

export const forceLeaveRound = (controller: PlayerController, rounds: SweetPieRound[], player: number): void => {
  const round = getPlayerCurrentRound(rounds, player);
  if (round && round.hallPointName) {
    controller.setSpawnPoint(player, round.hallPointName);
    controller.teleport(player, round.hallPointName);
  }
  round?.players?.delete(player);
}

export const determineDeathMatchWinners = (round: SweetPieRound): number[] => {
  if (round.players) {
    let bestKills = -1;
    for (const [, playerState] of round.players) {
      if (playerState.kills && playerState.kills > bestKills) {
        bestKills = playerState.kills;
      }
    }
    if (bestKills !== -1) {
      let bestPlayers = new Array<number>();
      for (const [player, playerState] of round.players) {
        if (playerState.kills === bestKills) {
          bestPlayers.push(player);
        }
      }
      return bestPlayers;
    }
  }
  return [];
}
