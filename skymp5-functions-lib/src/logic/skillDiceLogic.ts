import { ChatMessage, ChatText } from '../props/chatProperty';
import { PlayerController } from './PlayerController';
import { getName } from '../mpApiInteractor';
import { getPossesedSkills } from './skillMenuLogic';
import { EvalProperty } from '..//props/evalProperty';
import { Ctx } from '../types/ctx';
import { Mp } from '../types/mp';
import { weaponTypes } from './skillDiceData';

declare const ctx: Ctx;
declare const eventString: string;
declare const weaponKeywords: {
  keyword: string;
  name: string;
}[];
declare const mp: Mp;

export const skillDice = (
  actorId: number,
  controller: PlayerController,
  neighbors: number[],
  inputText: string,
  masterApiId: number | undefined
) => {
  const actorName = getName(actorId);
  const [_, action, type, value] = inputText.split(' ');
  const colors: {
    [key: string]: string;
  } = {
    yellow: '#BDBD7D',
    white: '#FFFFFF',
    green: '#5DAD60',
    red: '#F78C8C',
    blue: '#7175D6',
    purple: '#9159B6',
  };
  let text: ChatText[] = [];
  switch (action) {
    case 'init':
      const { possessedSkills } = getPossesedSkills(actorId);
      // const equipment = mp.get(actorId, 'equipment');
      // console.log(equipment);
      EvalProperty.eval(
        actorId,
        () => {
          const player = ctx.sp.Game.getPlayer()!;
          // ctx.sp.printConsole(player.getEquippedWeapon(false)?.getWeaponType())
          const equippedWeapons = [] as string[];
          weaponKeywords.forEach((type) => {
            const keyword = ctx.sp.Keyword.getKeyword(type.keyword);
            if (player.wornHasKeyword(keyword)) {
              equippedWeapons.push(type.name);
            }
          });
          // ctx.sp.printConsole(equippedWeapons);
          const src = `
          window.dispatchEvent(new CustomEvent('initSkillDices', { detail: { skills: ${eventString}, weapons: ${JSON.stringify(equippedWeapons)}}}))
          `;
          ctx.sp.browser.executeJavaScript(src);
        },
        { eventString: JSON.stringify(possessedSkills), weaponKeywords: weaponTypes }
      );
      break;
    case 'initiative':
      text = [
        {
          text: `${actorName} имеет инциативу `,
          color: colors['yellow'],
          type: ['plain'],
        },
        {
          text: `- ${Math.floor(Math.random() * 100 + 1)}`,
          color: colors['white'],
          type: ['plain'],
        },
      ];
      break;
    case 'heal':
      text = [
        {
          text: `${actorName} восстанавливает единицу здоровья`,
          color: colors['green'],
          type: ['plain'],
        },
      ];
      break;
    case 'self-attack':
      text = [
        {
          text: `${actorName} теряет единицу здоровья`,
          color: colors['red'],
          type: ['plain'],
        },
      ];
      break;
    case 'heal':
      text = [
        {
          text: `${actorName} восстанавливает единицу здоровья`,
          color: colors['green'],
          type: ['plain'],
        },
      ];
      break;
    case 'magic':
      const magicNames = {
        conjuration: 'колдовства',
        destruction: 'разрушения',
        restoration: 'восстановления',
        alteration: 'изменения',
        illusion: 'иллюзии',
      } as { [key: string]: string };
      if (type === 'select') {
        text = [
          {
            text: `${actorName} использует магию ${magicNames[value]}`,
            color: colors['purple'],
            type: ['plain'],
          },
        ];
        break;
      }
      text = [
        {
          text: `${actorName} использует магию ${magicNames[type]} `,
          color: colors['purple'],
          type: ['plain'],
        },
        {
          text: `- ${Math.floor(Math.random() * 20 + 1) + +value}`,
          color: colors['white'],
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
