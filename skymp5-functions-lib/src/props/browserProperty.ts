import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';

declare const mp: Mp;
declare const ctx: Ctx;

export class BrowserProperty {
  static init() {
    mp.makeProperty('browserVisible', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideBrowserVisibleUpdate()).getText(),
      updateNeighbor: '',
    });
    mp.makeProperty('browserFocused', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideBrowserFocusedUpdate()).getText(),
      updateNeighbor: '',
    });
  }

  static setVisible(actorId: number, visible = true) {
    mp.set(actorId, 'browserVisible', visible);
  }

  static setFocused(actorId: number, focused = true) {
    mp.set(actorId, 'browserFocused', focused);
  }

  private static clientsideBrowserFocusedUpdate() {
    return () => {
      // Focused state is forced: we do not allow user to escape from dialog windows, etc
      if (ctx.value === undefined || (ctx.state.lastBrowserFocused === ctx.value && !ctx.value)) {
        return;
      }
      ctx.sp.browser.setFocused(ctx.value);
      ctx.state.lastBrowserFocused = ctx.value;
    };
  }

  private static clientsideBrowserVisibleUpdate() {
    return () => {
      if (ctx.value === undefined || ctx.state.lastBrowserVisible === ctx.value) {
        return;
      }
      ctx.sp.browser.setVisible(ctx.value);
      ctx.state.lastBrowserVisible = ctx.value;
    };
  }
}
