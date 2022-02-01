import { SweetPieRound } from "./SweetPieRound";

export type PlayerController = {
  setSpawnPoint(player: number, pointName: string): void;
  teleport(player: number, pointName: string): void;
  showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]): void;
  sendChatMessage(actorId: number, text: string): void;
  quitGame(actorId: number): void;
  getName(actorId: number): string;
  addItem(actorId: number, itemId: number, count: number): void;
  getRoundsArray(): SweetPieRound[];
  setRoundsArray(rounds: SweetPieRound[]): void;
  getOnlinePlayers(): number[];
}
