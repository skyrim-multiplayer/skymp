export type Percentages = {
  health?: number;
  magicka?: number;
  stamina?: number;
}

export type PlayerController = {
  showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]): void;
  sendChatMessage(actorId: number, text: string): void;
  quitGame(actorId: number): void;
  getName(actorId: number): string;
  getProfileId(playerActorId: number): number;
  addItem(actorId: number, itemId: number, count: number): void;
  getOnlinePlayers(): number[];
  setPercentages(actorId: number, percentages: Percentages): void;
  getPercentages(actorId: number): Percentages;
  getScriptName(refrId: number): string;
  isTeleportActivator(refrId: number): boolean;
  updateCustomName(formDesc: string, name: string): void;
}
