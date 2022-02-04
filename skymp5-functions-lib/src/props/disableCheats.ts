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
      
      // Disable setav
      ctx.sp.findConsoleCommand("setAV").execute = () => false;
      
      // Disable modav
      ctx.sp.findConsoleCommand("modAV").execute = () => false;

      // Disable tcl
      ctx.sp.findConsoleCommand("ToggleCollision").execute = () => false;

      // Disable tgm
      ctx.sp.findConsoleCommand("ToggleGodMode").execute = () => false;
    };
  }
}
