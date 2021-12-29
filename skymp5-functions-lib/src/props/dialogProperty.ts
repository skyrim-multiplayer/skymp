import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { BrowserProperty } from './browserProperty';

type DialogValue = ['messageBox', string, string, string[]] | null;
type DialogState = { dialogPrevValue?: DialogValue };

declare const mp: Mp;
declare const ctx: Ctx;

export type DialogResponse = { actorId: number; buttonIndex: number; dialogId: number };
export type DialogResponseHandler = (response: DialogResponse) => boolean;

export class DialogProperty {
  static init() {
    mp.makeProperty('dialog', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideUpdateOwner()).getText(),
      updateNeighbor: '',
    });
    mp.makeProperty('dialogId', {
      isVisibleByOwner: false,
      isVisibleByNeighbors: false,
      updateOwner: '',
      updateNeighbor: '',
    });
    mp.makeEventSource('_onDialogResponse', new FunctionInfo(this.clientsideInitDialogResponse()).getText());
    mp['_onDialogResponse'] = this.onDialogResponse;
  }

  public static showMessageBox(actorId: number, dialogId: number, caption: string, text: string, buttons: string[]) {
    const value: DialogValue = ['messageBox', caption, text, buttons];
    mp.set(actorId, 'dialogId', dialogId);
    mp.set(actorId, 'dialog', value);

    BrowserProperty.setFocused(actorId);
  }

  public static clearDialog(actorId: number) {
    mp.set(actorId, 'dialog', null);
    mp.set(actorId, 'dialogId', null);
  }

  public static setDialogResponseHandler(handler: DialogResponseHandler) {
    DialogProperty.dialogResponseHandler = handler;
  }

  private static onDialogResponse(actorId: number, ...args: unknown[]) {
    const dialogId = mp.get(actorId, 'dialogId');
    if (typeof dialogId !== 'number') {
      return;
    }

    if (args[0] !== 'buttonClick' || typeof args[1] !== 'number') {
      return;
    }

    const buttonIndex = args[1];
    const success =
      !DialogProperty.dialogResponseHandler || DialogProperty.dialogResponseHandler({ actorId, buttonIndex, dialogId });
    if (success) {
      DialogProperty.clearDialog(actorId);
      BrowserProperty.setFocused(actorId, false);
    }
  }

  private static clientsideInitDialogResponse() {
    return () => {
      ctx.sp.on('browserMessage', (event) => {
        if (event.arguments[0] === 'buttonClick') {
          ctx.sendEvent(...event.arguments);
        }
      });
    };
  }

  private static clientsideUpdateOwner() {
    return () => {
      const ctxTyped = (ctx: Ctx): ctx is Ctx<DialogState, DialogValue> => true;
      if (!ctxTyped(ctx)) {
        return;
      }
      if (ctx.value === ctx.state.dialogPrevValue) {
        return;
      }
      ctx.state.dialogPrevValue = ctx.value;

      // Please keep up-to-date with impl in refreshWidgets.ts
      const refreshWidgets = 'window.skyrimPlatform.widgets.set((window.chat || []).concat(window.dialog || []));';

      if (!ctx.value) {
        let src = '';
        src += 'window.dialog = [];';
        src += refreshWidgets;
        return ctx.sp.browser.executeJavaScript(src);
      }

      switch (ctx.value[0]) {
        case 'messageBox':
          const [, caption, text, buttons] = ctx.value;
          let src = '';
          src += `tmp = { type: 'form', caption: '${caption}', elements: []};`;
          src += `tmp.elements.push({ type: 'text', text: '${text}' });`;
          buttons.forEach((button, i) => {
            src += `tmp.elements.push({ type: 'button', text: '${button}', tags: ['BUTTON_STYLE_FRAME'], click: () => window.skyrimPlatform.sendMessage('buttonClick', ${i})});`;
          });
          src += 'window.dialog = [tmp];';
          src += refreshWidgets;
          ctx.sp.browser.executeJavaScript(src);
          break;
        default:
          const never: never = ctx.value[0];
          throw new Error(`This value shouldn't exist: '${never}'`);
      }
    };
  }

  private static dialogResponseHandler?: DialogResponseHandler;
}
