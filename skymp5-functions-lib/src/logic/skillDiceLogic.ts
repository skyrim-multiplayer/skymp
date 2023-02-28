import { ChatMessage, ChatText } from '../props/chatProperty';
import { PlayerController } from './PlayerController';
import { getName } from '../mpApiInteractor';

export const skillDice = (
  actorId: number,
  controller: PlayerController,
  neighbors: number[],
  inputText: string,
  masterApiId: number | undefined
) => {
  const name = getName(actorId);
  const [_, arg1] = inputText.split(' ');
  const colors: {
    [key: string]: string;
  } = {
    yellow: '#BDBD7D',
    white: '#FFFFFF',
    green: '#5DAD60',
    red: '#F78C8C',
    20: '#7175D6',
    100: '#9159B6',
  };
  let text: ChatText[] = [];
  switch (arg1) {
    case 'initiative':
      const result = Math.floor(Math.random() * 100 + 1);
      text = [
        {
          text: `${name} имеет инциативу `,
          color: colors['yellow'],
          type: ['plain'],
        },
        {
          text: `- ${result}`,
          color: colors['white'],
          type: ['plain'],
        },
      ];
      break;
    case 'heal':
      text = [
        {
          text: `${name} восстанавливает единицу здоровья`,
          color: colors['green'],
          type: ['plain'],
        },
      ];
      break;
    case 'self-attack':
      text = [
        {
          text: `${name} теряет единицу здоровья`,
          color: colors['red'],
          type: ['plain'],
        },
      ];
      break;
    case 'heal':
      text = [
        {
          text: `${name} восстанавливает единицу здоровья`,
          color: colors['green'],
          type: ['plain'],
        },
      ];
      break;
    default:
      break;
  }
  const message = new ChatMessage(actorId, masterApiId || controller.getProfileId(actorId), text, 'dice', controller);
  for (const neighbor of neighbors) {
    controller.sendChatMessage(neighbor, message);
  }
};
