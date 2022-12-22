import { getActorDistanceSquared, getName } from '../mpApiInteractor';
import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { ChatSettings } from '../types/settings';
import { FunctionInfo } from '../utils/functionInfo';
import { parseMessage } from '../utils/parseChatMessage';
import { sqr } from '../utils/sqr';
import { EvalProperty } from './evalProperty';
import { refreshWidgetsJs } from './refreshWidgets';

type ChatValue = { show: boolean };
type ChatState = { chatPrevValue?: ChatValue; chatIsInputHidden?: boolean };

declare const mp: Mp;
declare const ctx: Ctx;
declare const messageString: string;
declare const refreshWidgets: string;

const whisperDistanceCoeff = 0.25;
const shoutDistanceCoeff = 2.4;

export type ChatText = {
  opacity?: string;
  color: string;
  text: string;
  href?: string;
  type: 'nonrp' | 'action' | 'whisper' | 'shout' | 'plain';
};

export interface IChatMessage {
  opacity: number;
  sender: {
    masterApiId: number;
    gameId: number;
  };
  category: 'dice' | 'nonrp' | 'system' | 'plain';
  text: ChatText[];
}

export type ChatInput = { actorId: number; inputText: string };
export type ChatInputHandler = (input: ChatInput) => void;

export type ChatNeighbor = { actorId: number; opacity: number };

const colorsArray = [
  '#5DAD60',
  '#62C985',
  '#7175D6',
  '#71D0D6',
  '#93AD5D',
  '#A062C9',
  '#BDBD7D',
  '#D76464',
  '#F78C8C',
  '#F78CD9',
];

export const getColorByNickname = (name: string) => {
  let result = 0;
  for (let i = 0; i < name.length; i++) {
    result += name.charCodeAt(i);
  }
  return colorsArray[result % colorsArray.length];
};

export class ChatMessage {
  private category: 'dice' | 'nonrp' | 'system' | 'plain';
  private text: ChatText[];
  private rawText?: string;
  private sender: {
    masterApiId: number;
    gameId: number;
    name?: string;
  };

  constructor(
    actorId: number,
    masterApiId: number,
    text: string | ChatText[],
    category: 'dice' | 'nonrp' | 'system' | 'plain' = 'plain'
  ) {
    this.sender = {
      masterApiId,
      gameId: actorId,
    };
    this.category = category;

    console.log(typeof text)

    if (typeof text === 'string') {
      this.sender.name = getName(actorId);
      this.text = parseMessage(text);
    } else {
      this.text = text as ChatText[];
    }
  }

  static system(text: string | ChatText[]) {
    return new this(0, 0, text, 'system');
  }
  
  // TBD
  public toUser(actorId: number): IChatMessage | false {
    const distance = getActorDistanceSquared(actorId, this.sender.gameId);
    const chatSettings = (mp.getServerSettings().sweetpieChatSettings as ChatSettings) ?? {};
    const hearingRadius =
      chatSettings.hearingRadiusNormal !== undefined ? sqr(chatSettings.hearingRadiusNormal) : sqr(2000);

    let texts: ChatText[] = this.text

    console.log(texts)
    if (this.category !== 'system') {
      texts = texts.reduce<ChatText[]>((filtered, text) => {
        if (text.type === 'shout' && distance < hearingRadius * shoutDistanceCoeff) {
          filtered.push({opacity: (((hearingRadius * shoutDistanceCoeff) - distance) / (hearingRadius * shoutDistanceCoeff)).toFixed(3), ...text})
        } else if (text.type === 'whisper' && distance < hearingRadius * whisperDistanceCoeff) {
          filtered.push({opacity: (((hearingRadius * whisperDistanceCoeff) - distance) / (hearingRadius * whisperDistanceCoeff)).toFixed(3), ...text})
        } else if (distance < hearingRadius) {
          filtered.push({opacity: (((hearingRadius) - distance) / (hearingRadius)).toFixed(3), ...text})
        }
        return filtered
      }, [])
    }

    if (texts.length === 0)
      return false
      
    if (this.sender.name) {
      texts = [
        {
          type: 'plain',
          text: `${this.sender.name}: `,
          color: getColorByNickname(this.sender.name)
        },
        ...texts
      ]
    }
    console.log(JSON.stringify(texts), actorId, this.sender.gameId)
    return {
      opacity: 1,
      sender: {
        gameId: this.sender.gameId,
        masterApiId: this.sender.masterApiId
      },
      text: texts,
      category: this.category
    };
  }
}

export const createSystemMessage = (text: string | ChatText[]): ChatMessage => {
  return ChatMessage.system(text);
};

export class ChatProperty {
  static init() {
    mp.makeProperty('chat', {
      isVisibleByOwner: true,
      isVisibleByNeighbors: false,
      updateOwner: new FunctionInfo(this.clientsideUpdateOwner()).getText({ refreshWidgets: refreshWidgetsJs }),
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
    value.show = show;
    mp.set(actorId, 'chat', value);
  }

  public static sendChatMessage(actorId: number, message: ChatMessage) {
    const messageToUser = message.toUser(actorId)
    if (messageToUser) {
      EvalProperty.eval(
        actorId,
        () => {
          let src = '';
          src += `window.chatMessages = window.chatMessages.slice(-49) || [];`;
          src += `window.chatMessages.push(${messageString});`;
          src += refreshWidgets;
          src += `if (window.scrollToLastMessage) { window.scrollToLastMessage(); }`;
          ctx.sp.browser.executeJavaScript(src);
        },
        { messageString: JSON.stringify(messageToUser), refreshWidgets: refreshWidgetsJs }
      );
    }
  }

  public static setChatInputHandler(handler: ChatInputHandler) {
    this.chatInputHandler = handler;
  }

  private static clientsideUpdateOwner() {
    return () => {
      const isInputHidden = !ctx.sp.browser.isFocused() || (ctx.get && ctx.get('dialog'));

      if (ctx.value === ctx.state.chatPrevValue && isInputHidden === ctx.state.chatIsInputHidden) {
        return;
      }
      ctx.state.chatPrevValue = ctx.value;
      ctx.state.chatIsInputHidden = isInputHidden;

      if (!ctx.value || !ctx.value.show) {
        let src = '';
        src += 'window.chat = [];';
        src += refreshWidgets;
        return ctx.sp.browser.executeJavaScript(src);
      }

      let src = '';
      src += `window.chatMessages = window.chatMessages || [];`; // Need to save chatMessages reference in chat[0]
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
    return () => {
      ctx.sp.on('browserMessage', (event) => {
        if (event.arguments[0] === 'chatInput') {
          ctx.sendEvent(...event.arguments);
        }
      });
    };
  }

  private static chatInputHandler: ChatInputHandler = () => {};
}
