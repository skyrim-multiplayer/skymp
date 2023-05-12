import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

export class DisableCheats {
  static init() {
    if (1 || mp.getServerSettings()['sweetPieAllowCheats']) {
      return;
    }
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

      const commandsToDisable: string[] = [
        "modAV",
        "forceAV",
        "setAV",
        "SetScale",
        "ToggleCollision",
        "ToggleControlsDriven",
        "ToggleGodMode", // Disable unlimited carry weight
        "tg",
        "tmm",
        "tfc",
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
        "SAQ",
        "CAQS",
        "openactorcontainer",
      ];

      // Disable commands from commandsToDisable
      commandsToDisable.forEach((command) => {
        const cmd = ctx.sp.findConsoleCommand(command);
        if (!cmd) {
          ctx.sp.printConsole(`Can't find command ${command}`);
          return;
        }
        cmd.execute = () => false;
      })
    };
  }
}
