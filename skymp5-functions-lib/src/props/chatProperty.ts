import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { BrowserProperty } from './browserProperty';
import { EvalProperty } from './evalProperty';

type ChatValue = { show: boolean };
type ChatState = { chatPrevValue?: ChatValue; chatIsInputHidden?: boolean };

declare const mp: Mp;

export type ChatInput = { actorId: number; inputText: string };
export type ChatInputHandler = (input: ChatInput) => void;

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
    ChatProperty.chatInputHandler({ actorId, inputText });
  }

  public static showChat(actorId: number, show = true) {
    const value: ChatValue = mp.get(actorId, 'chat') || { show: false };
    if (value.show !== show) {
      value.show = show;
      mp.set(actorId, 'chat', value);
    }
  }

  public static sendChatMessage(actorId: number, message: string) {
    EvalProperty.eval(
      actorId,
      (ctx: Ctx) => {
        // Please keep up-to-date with impl in dialogProperty.ts
        const refreshWidgets = 'window.skyrimPlatform.widgets.set((window.chat || []).concat(window.dialog || []));';
        let src = '';
        const htmlEscapes: Record<string, string> = {
          '"': '\\"',
          "'": "\\'",
          '\\': '\\\\',
        };
        const htmlEscaper = /[&<>"'\\\/]/g;
        const msg = message.replace(htmlEscaper, (match) => htmlEscapes[match]);
        src += `window.chatMessages = window.chatMessages || [];`;
        src += `window.chatMessages.push("${msg}");`;
        src += refreshWidgets;
        ctx.sp.browser.executeJavaScript(src);
      },
      { message }
    );
  }

  public static setChatInputHandler(handler: ChatInputHandler) {
    this.chatInputHandler = handler;
  }

  private static clientsideUpdateOwner() {
    return (ctx: Ctx<ChatState, ChatValue>) => {
      const isInputHidden = !ctx.sp.browser.isFocused() || (ctx.get && ctx.get('dialog') !== null);

      if (ctx.value === ctx.state.chatPrevValue && isInputHidden === ctx.state.chatIsInputHidden) {
        return;
      }
      ctx.state.chatPrevValue = ctx.value;
      ctx.state.chatIsInputHidden = isInputHidden;

      // Please keep up-to-date with impl in dialogProperty.ts
      const refreshWidgets = 'window.skyrimPlatform.widgets.set((window.chat || []).concat(window.dialog || []));';

      if (!ctx.value || !ctx.value.show) {
        let src = '';
        src += 'window.chat = [];';
        src += refreshWidgets;
        return ctx.sp.browser.executeJavaScript(src);
      }

      let src = '';
      src += 'window.chat = [{}];';
      src += 'window.chat[0].type = "chat";';
      src += 'window.chat[0].messages = window.chatMessages;';
      src += 'window.chat[0].send = (text) => window.skyrimPlatform.sendMessage("chatInput", text);';
      src += `window.chat[0].isInputHidden = ${isInputHidden};`;
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

  private static chatInputHandler: ChatInputHandler = () => {};
}
