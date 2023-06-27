export type SweetPieMap = {
  safePointName: string;
  mainSpawnPointName?: string;
  safePlaceLeaveDoors?: string[];
  safePlaceEnterDoors?: string[];
  leaveRoundDoors?: string[];
  playerRestoreActivators?: string[];
  playerRestoreWaitTime?: number;
  spawnPointNames?: string[];
  enabled?: boolean;
}
