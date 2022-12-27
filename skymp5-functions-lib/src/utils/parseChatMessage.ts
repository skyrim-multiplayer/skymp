import { ChatText, ChatTextType } from '../props/chatProperty';

export interface IChatMessage {
  opacity: number;
  sender: {
    masterApiId: number;
    gameId: string;
  };
  category: 'dice' | 'nonrp' | 'system' | 'plain';
  text: ChatText[];
}

type delimitersMap = {
  [index: string]: {
    double?: true;
    close: string;
    type: ChatTextType;
    color: string;
    canBeNested?: boolean;
  };
};

export const parseChatMessage = (text: string): ChatText[] => {
  const map: delimitersMap = {
    '(': {
      double: true,
      close: ')',
      type: 'nonrp',
      color: '#91916D',
    },
    '*': {
      close: '*',
      type: 'action',
      color: '#CFAA6E',
    },
    '%': {
      close: '%',
      type: 'whisper',
      color: '#A062C9',
    },
    '№': {
      close: '№',
      type: 'shout',
      color: '#F78C8C',
      canBeNested: false,
    },
  };

  const stack: string[] = [];

  const texts: ChatText[] = [];

  let lastIndex = 0;

  let currentType: ChatTextType[] = [];

  for (let i = 0; i < text.length; i++) {
    let char = text[i];
    if (char in map) {
      if (char === stack[stack.length - 1]) {
        stack.pop();
        texts.push({
          text: text.slice(lastIndex, i),
          color: map[char].color,
          type: [...currentType],
        });
        lastIndex = i;
        currentType.pop();
      } else {
        if (map[char].double) {
          if (char !== text[i + 1]) {
            continue;
          } else {
            i += 1;
          }
        }
        if (map[char].canBeNested === false && (stack.length !== 0 || currentType.length !== 0)) {
          continue;
        }

        const prevColor = currentType.length > 0 ? map[stack[0]].color : '#FFFFFF';

        stack.push(char);

        let t_this = 0;
        let t_prev = 0;

        if (stack[0] && map[stack[stack.length - 1]].double) {
          t_this += 1;
        }
        if (stack[1] && map[stack[stack.length - 2]].double) {
          t_prev += 1;
        }

        texts.push({
          text: text.slice(lastIndex + t_this + t_prev, i - t_this),
          color: prevColor,
          type: currentType.length > 0 ? [...currentType] : ['plain'],
        });
        currentType.push(map[char].type);
        lastIndex = i;
      }
    } else {
      const closing = Object.keys(map).find((key) => map[key].close === char);
      if (closing) {
        if (map[closing].double) {
          if (map[closing].close !== text[i + 1]) {
            continue;
          } else {
            i += 1;
          }
        }
        if (closing === stack[stack.length - 1]) {
          stack.pop();
          texts.push({
            text: text.slice(lastIndex + 1, i - 1),
            color: map[closing].color,
            type: [...currentType],
          });
          currentType.pop();
          lastIndex = i + 1;
        }
      }
    }
  }
  texts.push({
    type: ['plain'],
    text: text.slice(lastIndex),
    color: '#FFFFFF',
  });
  texts.forEach((msg) => {
    msg.text = msg.text.replace(/\%|\№|\*/gi, '');
  });
  return texts.filter((msg) => msg.text !== '');
};
