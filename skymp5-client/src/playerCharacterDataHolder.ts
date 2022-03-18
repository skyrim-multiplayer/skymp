import { Game } from "skyrimPlatform";
import { UtilsObjectReference } from "./utilsObjectReference";

export class PlayerCharacterDataHolder {
  static updateData() {
    const player = Game.getPlayer();
    if (!player) return;
    
    this.inJumpState = player.getAnimationVariableBool("bInJumpState");
    this.worldOrCell = UtilsObjectReference.getWorldOrCell(player);
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
