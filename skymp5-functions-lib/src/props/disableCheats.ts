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

      const commandsToDisable: string[] = [
        "modAV",
        "forceAV",
        "setAV",
        "ToggleCollision",
        "ToggleGodMode", // Disable unlimited carry weight
        "ToggleFreeCamera",
      ]

      // Disable commands from commandsToDisable
      commandsToDisable.forEach((command) => {
        ctx.sp.findConsoleCommand(command).execute = () => false;
      })
    };
  }
}
