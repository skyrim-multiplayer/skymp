import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { BrowserProperty } from './browserProperty';

type ChatValue = { show: boolean; lastMessage?: string; n?: number };
type ChatState = { chatPrevValue?: ChatValue; chatIsInputHidden?: boolean; messages?: string[]; n?: number };

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
    const value: ChatValue = mp.get(actorId, 'chat') || { show: false };
    value.lastMessage = message;
    value.n = value.n ? value.n + 1 : 1;
    mp.set(actorId, 'chat', value);
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
      src += 'window.chat[0].messages = [];';
      src += 'window.chat[0].send = (text) => window.skyrimPlatform.sendMessage("chatInput", text);';
      src += `window.chat[0].isInputHidden = ${isInputHidden};`;

      ctx.state.messages = ctx.state.messages || [];
      if (ctx.state.n !== ctx.value.n && ctx.value.lastMessage) {
        ctx.state.n = ctx.value.n;
        ctx.state.messages.push(ctx.value.lastMessage);
      }
      const htmlEscapes: Record<string, string> = {
        '"': '\\"',
        "'": "\\'",
        '\\': '\\\\',
      };
      const htmlEscaper = /[&<>"'\\\/]/g;
      for (const message of ctx.state.messages) {
        const msg = message.replace(htmlEscaper, (match) => htmlEscapes[match]);
        src += `window.chat[0].messages.push("${msg}");`;
      }

      src += refreshWidgets;

      // src.split(';').forEach(ctx.sp.printConsole);

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
