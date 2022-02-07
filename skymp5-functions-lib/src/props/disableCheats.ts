import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

export class DisableCheats {
  static init() {
    mp.makeEventSource('_onHacks', new FunctionInfo(this.clientsideInit()).getText());
    mp['_onHacks'] = () => { };
  }

  private static clientsideInit() {
    return () => {
      // Disable Ctrl+PrintScreen
      if (ctx.sp.storage["disableCtrlPrtScnHotkeyCalled"] !== true) {
        ctx.sp.storage["disableCtrlPrtScnHotkeyCalled"] = true;
        ctx.sp.disableCtrlPrtScnHotkey();
      }

      // This code fails to disable some commands
      const commandsToDisable: string[] = [
        "modAV",
        "forceAV",
        "setAV",
        "SetScale",
        "ToggleCollision",
        "ToggleControlsDriven",
        "ToggleGodMode", // Disable unlimited carry weight
        "ToggleFreeCamera",
        "ToggleImmortalMode",
        "ToggleMotionDriven",
        "ToggleTrees",
        "ToggleScripts",
        "ToggleCellNode",
        "ToggleSky",
        "SetGlobalTimeMultiplier",
        "ToggleWaterSystem",
        "ForceWeather",
        "SetWeather",
        "SexChange",
        "ToggleCombatAI",
        "ToggleAI",
        "SetGameSetting",
        "SetPos",
        "SetAngle",
        "ToggleFogOfWar",
      ]

      // Disable commands from commandsToDisable
      commandsToDisable.forEach((command) => {
        ctx.sp.findConsoleCommand(command).execute = () => false;
      })
    };
  }
}
