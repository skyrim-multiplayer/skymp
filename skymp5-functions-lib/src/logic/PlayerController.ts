import { SweetPieRound } from "./SweetPieRound";

export type Percentages = {
  health?: number;
  magicka?: number;
  stamina?: number;
}

export type Counter = 'finishedDeathmatches' | 'everydayStart' | 'secondsToday' | 'lastExtraRewardDay';

export type PlayerController = {
  setSpawnPoint(player: number, pointName: string): void;
  teleport(player: number, pointName: string): void;
  showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]): void;
  sendChatMessage(actorId: number, text: string): void;
  quitGame(actorId: number): void;
  getName(actorId: number): string;
  getProfileId(playerActorId: number): number;
  addItem(actorId: number, itemId: number, count: number): void;
  getRoundsArray(): SweetPieRound[];
  setRoundsArray(rounds: SweetPieRound[]): void;
  getOnlinePlayers(): number[];
  setPercentages(actorId: number, percentages: Percentages): void;
  getPercentages(actorId: number): Percentages;
  getScriptName(refrId: number): string;
  isTeleportActivator(refrId: number): boolean;
  updateCustomName(formDesc: string, name: string): void;
  incrementCounter(actorId: number, counter: Counter, by?: number): number;
}
