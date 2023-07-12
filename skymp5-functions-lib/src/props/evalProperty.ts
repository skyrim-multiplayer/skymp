import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

type EvalState = { evalGreatestId?: number };
type EvalValue = { commands: Array<{ code: string; id: number }>; nextId: 0 };

export class EvalProperty {
  static init() {
    mp.makeProperty('eval', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideUpdateOwner()).getText(),
      updateNeighbor: '',
    });
    mp.makeEventSource('_onEvalFinish', new FunctionInfo(this.clientsideInitEvalFinish()).getText());
    mp['_onEvalFinish'] = this.onEvalFinish;
  }

  static eval(actorId: number, f: (ctx: Ctx, ...args: any[]) => void, args?: Record<string, unknown>) {
    const baseDesc = mp.get(actorId, "baseDesc");
    const baseId = mp.getIdFromDesc(baseDesc);
    if (baseId !== 0x7 && baseId !== 0) return;

    const code = new FunctionInfo(f).getText(args);
    const value: EvalValue = mp.get(actorId, 'eval') || { commands: [], nextId: 0 };
    value.commands.push({ code, id: value.nextId });
    value.nextId++;
    mp.set(actorId, 'eval', value);
  }

  private static onEvalFinish(actorId: number, ...args: unknown[]) {
    if (typeof args[0] === 'number') {
      const greatestExecutedId = args[0];
      const value: EvalValue = mp.get(actorId, 'eval') || { commands: [], nextId: 0 };
      value.commands = value.commands.filter((command) => command.id > greatestExecutedId);
      mp.set(actorId, 'eval', value);
    }
  }

  private static clientsideUpdateOwner() {
    return () => {
      if (!ctx.value) {
        return;
      }

      if (typeof ctx.state.evalGreatestId !== 'number') {
        ctx.state.evalGreatestId = -1;
      }

      for (const command of ctx.value.commands) {
        if (command.id > ctx.state.evalGreatestId) {
          ctx.state.evalGreatestId = command.id;

          // Can't ctx.sendEvent here, so...
          ctx.sp.browser.executeJavaScript(
            `window.skyrimPlatform.sendMessage('evalFinish', ${ctx.state.evalGreatestId})`
          );
          eval(command.code);
        }
      }
    };
  }

  private static clientsideInitEvalFinish() {
    return () => {
      ctx.sp.on('browserMessage', (event) => {
        if (event.arguments[0] === 'evalFinish') {
          const evalGreatestId = event.arguments[1];
          ctx.sendEvent(evalGreatestId);
        }
      });
    };
  }
}
