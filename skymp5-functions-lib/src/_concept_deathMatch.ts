export interface GameModeListener {
  onPlayerJoin?: (actorId: number) => void;
  onPlayerLeave?: (actorId: number) => void;
  onPlayerDeath?: (targetActorId: number, killerActorId?: number) => void;
  everySecond?: () => void;
  onPlayerChatInput?: (actorId: number, inputText: string) => void;
  onPlayerDialogResponse?: (actorId: number, dialogId: number, buttonIndex: number) => void;
  onPlayerActivateObject?: (casterActorId: number, targetObjectDesc: string, isTeleportDoor: boolean) => 'continue' | 'blockActivation';
}

export interface GameModeController {
  showMessageBox: (actorId: number, dialogId: number, caption: string, text: string, buttons: string[]) => void;
}

export class RoundSystemBase2000 implements GameModeListener {

}

export class SweetPie1000 implements GameModeListener {

  public readonly portalQuit = '42f3f:SweetPie.esp';
  public readonly portalNeutral = '42f70:SweetPie.esp';
  public readonly portalRed = '42e96:SweetPie.esp';
  public readonly portalBlue = '42fc1:SweetPie.esp';

  public readonly dialogJoinDeathMatch = 1000;

  constructor(public readonly controller: GameModeController) { }

  onPlayerActivateObject(casterActorId: number, targetObjectDesc: string): 'continue' | 'blockActivation' {
    if (targetObjectDesc === this.portalNeutral) {
      this.controller.showMessageBox(casterActorId, this.dialogJoinDeathMatch, "DeathMatch", "Join DeathMatch?", ["Yes", "No"]);
      return 'blockActivation';
    }
    return 'continue';
  }

  onPlayerDialogResponse(actorId: number, dialogId: number, buttonIndex: number): void {
    if (dialogId === this.dialogJoinDeathMatch) {
      if (buttonIndex === 0) {
      }
    }
  }

  //private deathMatchRounds = new Array<RoundInfo>;
}