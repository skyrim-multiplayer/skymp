import { ChatText } from "../props/chatProperty";

type delimitersMap = {
  [index: string]: {
    double?: true;
    close: string;
    type: "nonrp" | "action" | "whisper" | "shout" | "plain";
    color: string;
  };
};

export const parseMessage = (text: string): ChatText[] => {
  const map: delimitersMap = {
    "(": {
      double: true,
      close: ")",
      type: "nonrp",
      color: "asd"
    },
    "*": {
      close: "*",
      type: "action",
      color: "asd"
    },
    "%": {
      close: "%",
      type: "whisper",
      color: "asd"
    },
    "№": {
      close: "№",
      type: "shout",
      color: "asd"
    }
  };

  const stack: string[] = [];

  const texts: ChatText[] = [];

  let lastIndex = 0;

  for (let i = 0; i < text.length; i++) {
    let char = text[i];
    if (char in map) {
      if (map[char].double && char !== text[i + 1]) {
        continue;
      } else {
        i += 1;
      }
      if (char === stack[stack.length - 1]) {
        stack.pop();
        texts.push({
          text: text.slice(lastIndex, i - 1),
          color: map[char].color,
          type: map[char].type
        });
        lastIndex = i;
      } else {
        stack.push(char);
        texts.push({
          text: text.slice(lastIndex, i - 1),
          color: "#FFFFFF",
          type: "plain"
        });
        lastIndex = i;
      }
    } else {
      const closing = Object.keys(map).find((key) => map[key].close === char);
      if (closing) {
        if (map[closing].double && map[closing].close !== text[i + 1]) {
          continue;
        } else {
          i += 1;
        }
        if (closing === stack[stack.length - 1]) {
          stack.pop();
          texts.push({
            text: text.slice(lastIndex + 1, i - 1),
            color: map[closing].color,
            type: map[closing].type
          });
          lastIndex = i + 1;
        }
      }
    }
  }
  texts.push({
    type: "plain",
    text: text.slice(lastIndex),
    color: "#FFFFFF"
  });
  return texts.filter((msg) => msg.text !== "");
};