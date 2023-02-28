import { ChatMessage, ChatText } from '../props/chatProperty';
import { PlayerController } from './PlayerController';
import { getName } from "../mpApiInteractor";

export const skillDice = (actorId: number, controller: PlayerController, neighbors: number[], inputText: string, masterApiId: number | undefined) => {
    const name = getName(actorId)
    const [_, arg1] = inputText.split(' ');
    const colors: {
        [key: string]: string
      } = {
        'yellow': '#BDBD7D',
        'white': '#FFFFFF',
        6: '#F78C8C',
        12: '#5DAD60',
        20: '#7175D6',
        100: '#9159B6',
      }
    let text: ChatText[] = []
    if (arg1 === 'initiative') {
        const result = Math.floor(Math.random() * (100) + 1);
        text = [
            {
              text: `${name} имеет инциативу `,
              color: colors['yellow'],
              type: ['plain']
            },
            {
              text: `- ${result}`,
              color: colors['white'],
              type: ['plain']
            }
          ]
    }
    const message = new ChatMessage(actorId, masterApiId || controller.getProfileId(actorId), text, 'dice', controller)
    for (const neighbor of neighbors) {
      controller.sendChatMessage(neighbor, message);
    } 
}