import { PlayerController } from '../logic/PlayerController';
import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { FunctionInfo } from '../utils/functionInfo';
import { parseChatMessage } from '../utils/parseChatMessage';
import { sqr } from '../mpApiInteractor';
import { EvalProperty } from './evalProperty';
import { refreshWidgetsJs } from './refreshWidgets';
import { ChatSettings } from '../types/settings';

type ChatValue = { show: boolean };

declare const mp: Mp;
declare const ctx: Ctx;
declare const messageString: string;
declare const refreshWidgets: string;

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

const filterMessages: FilterMessagesType = {
  shout: [
    {
      type: 'action',
      status: 'disabled',
    },
    {
      type: 'nonrp',
      status: 'distanceOnly',
      color: '#91916D',
    },
    {
      type: 'whisper',
      status: 'disabled',
    },
  ],
  whisper: [
    {
      type: 'action',
      status: 'enabled',
    },
    {
      type: 'nonrp',
      status: 'distanceOnly',
      color: '#91916D',
    },
  ],
  nonrp: [
    {
      type: 'action',
      status: 'inherit',
      color: '#91916D',
    },
    {
      type: 'shout',
      status: 'inherit',
      color: '#91916D',
    },
    {
      type: 'whisper',
      status: 'inherit',
      color: '#91916D',
    },
  ],
};

export type ChatTextType = 'nonrp' | 'action' | 'whisper' | 'shout' | 'plain';

export type ChatText = {
  opacity?: string;
  color: string;
  text: string;
  href?: string;
  type: ChatTextType[];
};

type FilterMessagesType = {
  [index: ChatTextType | string]: {
    type: ChatTextType;
    status: 'disabled' | 'enabled' | 'inherit' | 'distanceOnly';
    color?: string;
  }[];
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

export const getColorByNickname = (name: string) => {
  let result = 0;
  for (let i = 0; i < name.length; i++) {
    result += name.charCodeAt(i);
  }
  return colorsArray[result % colorsArray.length];
};

const calculateOpacity = (distance: number, max: number, minDistance: number, coeff: number): string => {
  // TODO: rename args
  if (distance <= minDistance * coeff) {
    return '1';
  }
  return ((max * coeff - distance + minDistance * coeff) / (max * coeff)).toFixed(5);
};

export class ChatMessage {
  private category: 'dice' | 'nonrp' | 'system' | 'plain';
  private text: ChatText[];
  private sender: {
    masterApiId: number;
    gameId: number;
    name?: string;
  };
  private controller: PlayerController | undefined;

  constructor(
    actorId: number,
    masterApiId: number,
    text: string | ChatText[],
    category: 'dice' | 'nonrp' | 'system' | 'plain' = 'plain',
    controller?: PlayerController
  ) {
    this.sender = {
      masterApiId,
      gameId: actorId,
    };
    this.category = category;
    if (controller) {
      this.controller = controller;
    }
    if (typeof text === 'string') {
      if (['plain', 'nonrp'].includes(category) && controller) {
        this.sender.name = controller.getName(actorId);
      }
      this.text = parseChatMessage(text);
    } else {
      this.text = text;
    }
  }

  static system(text: string | ChatText[], controller?: PlayerController) {
    return new this(0, 0, text, 'system', controller ?? undefined);
  }

  // TBD
  public toUser(actorId: number): IChatMessage | false {
    let texts: ChatText[] = this.text;

    if (['plain', 'nonrp', 'dice'].includes(this.category) && this.controller) {
      const chatSettings = this.controller.getServerSetting('sweetpieChatSettings') as ChatSettings ?? {};
      const hearingRadius =
        chatSettings['hearingRadiusNormal'] !== undefined ? sqr(chatSettings['hearingRadiusNormal']) : sqr(1900);
      const whisperDistanceCoeff =
        chatSettings['whisperDistance'] !== undefined ? chatSettings['whisperDistance'] : 0.1;
      const shoutDistanceCoeff = chatSettings['shoutDistance'] !== undefined ? chatSettings['shoutDistance'] : 2.45;
      const minDistanceToChange =
        chatSettings['minDistanceToChange'] !== undefined ? sqr(chatSettings['minDistanceToChange']) : sqr(500); // TODO: move const to config

      const distance = this.controller.getActorDistanceSquared(actorId, this.sender.gameId);
      texts = texts.reduce<ChatText[]>((filtered, text) => {
        const current = { ...text };
        if (text.type.length > 0) {
          // Apply filters
          for (let i = 0; i < text.type.length; i++) {
            const category = text.type[i];
            if (category in filterMessages) {
              const filter = filterMessages[category];
              for (let j = i; j < text.type.length; j++) {
                if (!filter[j]) {
                  continue;
                }
                if (text.type.includes(filter[j].type)) {
                  if (filter[j].status === 'disabled') {
                    continue;
                  }
                  if (filter[j].color !== undefined) {
                    current.color = filter[j].color!;
                  }
                  if (filter[j].status === 'enabled') {
                    current.type = current.type.filter((e) => e !== category);
                  }
                  if (filter[j].status === 'inherit') {
                    current.type = current.type.filter((e) => e !== filter[j].type);
                  }
                }
              }
            }
          }
        }

        // Apply distance filters

        if (
          (current.type.includes('shout') || current.type.includes('nonrp')) &&
          distance < hearingRadius * shoutDistanceCoeff
        ) {
          filtered.push({
            opacity: calculateOpacity(distance, hearingRadius, minDistanceToChange, shoutDistanceCoeff),
            ...current,
          });
          return filtered;
        } else if (current.type.includes('whisper')) {
          if (distance < hearingRadius * whisperDistanceCoeff) filtered.push({ opacity: '1', ...current });
          return filtered;
        } else if (distance < hearingRadius) {
          filtered.push({ opacity: calculateOpacity(distance, hearingRadius, minDistanceToChange, 1), ...current });
          return filtered;
        }
        return filtered;
      }, []);
    }

    if (texts.length === 0) {
      return false;
    }

    if (this.sender.name) {
      texts = [
        {
          type: ['plain'],
          text: `${this.sender.name}: `,
          color: getColorByNickname(this.sender.name),
        },
        ...texts,
      ];
    }

    return {
      opacity: 1,
      sender: {
        gameId: this.sender.gameId,
        masterApiId: this.sender.masterApiId,
      },
      text: texts,
      category: this.category,
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
    const messageToUser = message.toUser(actorId);
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

      // Add "Lost connection to the server" message
      const isConnected = ctx.sp.mpClientPlugin.isConnected();
      const wasConnected = ctx.state.isConnected;
      if (isConnected !== wasConnected) {
        ctx.state.isConnected = isConnected;
        let messageToUser: ChatMessage | undefined;
        if (isConnected === false) {
          messageToUser = {
            actorId: 0,
            masterApiId: 0,
            text: [{
              type: ['plain'],
              color: '#FFFFFF',
              text: "Lost connection to the server"
            }],
            category: "system"
          } as any;
        }
        else if (wasConnected === false && isConnected === true) {
          messageToUser = {
            actorId: 0,
            masterApiId: 0,
            text: [{
              type: ['plain'],
              color: '#FFFFFF',
              text: "Reconnected"
            }],
            category: "system"
          } as any;
        }
        if (messageToUser) {
          const messageString = JSON.stringify(messageToUser)
          let src = '';
          src += `window.chatMessages = window.chatMessages.slice(-49) || [];`;
          src += `window.chatMessages.push(${messageString});`;
          src += refreshWidgets;
          src += `if (window.scrollToLastMessage) { window.scrollToLastMessage(); }`;
          ctx.sp.browser.executeJavaScript(src);
        }
      }

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
