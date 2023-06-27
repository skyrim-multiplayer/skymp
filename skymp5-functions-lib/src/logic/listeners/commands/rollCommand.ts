import { ChatMessage, ChatText } from "../../../props/chatProperty";
import { Mp } from "../../../types/mp";
import { PlayerController } from "../../PlayerController";
import { Command, HandlerInput } from "./command";

export class RollCommand extends Command {
    constructor(mp: Mp, controller: PlayerController, name: string) {
        super(mp, controller, name);
    }

    handle(input: HandlerInput): void {
        const { actorId } = input;
        const name = this.controller.getName(actorId);
        const random: string[] = [];
        const [count, _, max]: number[] = input.inputText.slice(1).split(/(d|к)/g).map(str => parseInt(str));
        const colors: {
          [key: number]: string
        } = {
          2: '#BDBD7D',
          6: '#F78C8C',
          12: '#5DAD60',
          20: '#7175D6',
          100: '#9159B6',
        }
        for (let i = 0; i < count; i++) {
          if (i > 4) break;
          if (max === 2) {
            random.push(Math.floor(Math.random() * (max) + 1) === 2 ? 'успех' : 'неудача');
          } else {
            random.push(`${Math.floor(Math.random() * (max) + 1)}`);
          }
        }
        let text: ChatText[] = []
        if (max === 2) {
          text = [
            {
              text: `${name} подбрасывает монетку `,
              color: colors[max] ? colors[max] : '#9159B6',
              type: ['plain']
            },
            {
              text: `- ${random.join(', ')}`,
              color: '#FFFFFF',
              type: ['plain']
            }
          ]
        } else {
          text = [
            {
              text: `${name} бросает D${max} `,
              color: colors[max] ? colors[max] : '#9159B6',
              type: ['plain']
            },
            {
              text: `- ${random.join(', ')}`,
              color: '#FFFFFF',
              type: ['plain']
            }
          ]
        }
        const message = new ChatMessage(actorId, input.masterApiId || this.controller.getProfileId(actorId), text, 'dice', this.controller)
        for (const neighbor of input.neighbors) {
          this.controller.sendChatMessage(neighbor, message);
        } 
    }
}
