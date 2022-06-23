import { Game } from "skyrimPlatform";
import { ObjectReferenceEx } from "../extensions/objectReferenceEx";

export class PlayerCharacterDataHolder {
  static updateData() {
    const player = Game.getPlayer();
    if (!player) return;
    
    this.inJumpState = player.getAnimationVariableBool("bInJumpState");
    this.worldOrCell = ObjectReferenceEx.getWorldOrCell(player);
    this.crosshairRefId = Game.getCurrentCrosshairRef()?.getFormID() || 0;
  }

  static isInJumpState() {
    return this.inJumpState;
  }

  static getWorldOrCell() {
    return this.worldOrCell;
  }

  static getCrosshairRefId() {
    return this.crosshairRefId;
  }

  private static inJumpState = false;
  private static worldOrCell = 0;
  private static crosshairRefId = 0;
}
