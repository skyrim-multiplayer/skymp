import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { BrowserProperty } from './browserProperty';

type ChatValue = { show: boolean; messages: string[] };
type ChatState = { chatPrevValue?: ChatValue };

declare const mp: Mp;

export class ChatProperty {
  static init() {
    mp.makeProperty('chat', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideUpdateOwner()).getText(),
      updateNeighbor: '',
    });
    mp.makeEventSource('_onChatInput', new FunctionInfo(this.clientsideInitChatInput()).getText());
    mp['_onChatInput'] = this.onChatInput;
  }

  private static onChatInput(actorId: number, ...args: unknown[]) {
    if (args[0] !== 'chatInput' || typeof args[1] !== 'string') {
      return;
    }
    const [, inputText] = args;
    console.log({ inputText });
  }

  public static showChat(actorId: number, show = true) {
    const value: ChatValue = mp.get(actorId, 'chat') || { show: false, messages: [] };
    if (value.show !== show) {
      value.show = show;
      mp.set(actorId, 'chat', value);
    }
  }

  private static clientsideUpdateOwner() {
    return (ctx: Ctx<ChatState, ChatValue>) => {
      if (ctx.value === ctx.state.chatPrevValue) {
        return;
      }
      ctx.state.chatPrevValue = ctx.value;

      // Please keep up-to-date with impl in dialogProperty.ts
      const refreshWidgets = 'window.skyrimPlatform.widgets.set((window.chat || []).concat(window.dialog || []));';

      if (!ctx.value || !ctx.value.show) {
        let src = '';
        src += 'window.chat = [];';
        src += refreshWidgets;
        return ctx.sp.browser.executeJavaScript(src);
      }

      let src = '';
      src +=
        'window.chat = [{ type: "chat", messages: [], send: (text) => window.skyrimPlatform.sendMessage("chatInput", text) }];';
      src += refreshWidgets;
      ctx.sp.browser.executeJavaScript(src);
    };
  }

  private static clientsideInitChatInput() {
    return (ctx: Ctx) => {
      ctx.sp.on('browserMessage', (event) => {
        if (event.arguments[0] === 'chatInput') {
          ctx.sendEvent(...event.arguments);
        }
      });
    };
  }
}
