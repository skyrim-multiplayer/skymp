import { getName } from '../../../mpApiInteractor';
import { ChatText, ChatMessage } from '../../../props/chatProperty';
import { EvalProperty } from '../../../props/evalProperty';
import { Ctx } from '../../../types/ctx';
import { Mp } from '../../../types/mp';
import { PlayerController } from '../../PlayerController';
import { Command, HandlerInput } from './command';
import { getPossessedSkills } from './skillCommand';
import { ActorBase, Race } from 'skyrimPlatform';

export class SkillDiceCommand extends Command {
  constructor(mp: Mp, controller: PlayerController) {
    super(mp, controller, 'skill-dice');
  }

  handle(input: HandlerInput): void {
    const { actorId, controller, neighbors, inputText, masterApiId } = input;
    skillDice(actorId, controller, neighbors, inputText, masterApiId);
  }
}

declare const ctx: Ctx;
declare const eventString: string;
declare const weaponKeywords: {
  keyword: string;
  name: string;
}[];
declare const armorKeywords: {
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
  const [_, action, type, value, buff, hitPoints] = inputText.split(' ');
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
      const { possessedSkills } = getPossessedSkills(actorId);
      const inventory = mp.get(actorId, 'inventory');
      console.log(inventory);
      EvalProperty.eval(
        actorId,
        () => {
          const player = ctx.sp.Game.getPlayer()!;
          const leftHandedWeapon = player.getEquippedWeapon(true);
          const equippedWeapons = [] as string[];
          let armorType = null;
          const magicStaffIds = [
            0x07a5950b, 0x07a5950a, 0x07a59505, 0x07a59504, 0x07a6d92e, 0x07a5950e, 0x07a59510, 0x07a5950f, 0x07a5950d,
            0x07a5950c, 0x07005905, 0x07005906, 0x07a59507, 0x07a59506, 0x07a59509, 0x07a59508, 0x070da798, 0x070da795,
            0x070da796, 0x070da797, 0x070da799,
          ];
          const leftHandObject = player.getEquippedObject(0);
          if (leftHandObject && magicStaffIds.includes(leftHandObject.getFormID())) {
            equippedWeapons.push('magicstaff');
          }
          weaponKeywords.forEach((type) => {
            const keyword = ctx.sp.Keyword.getKeyword(type.keyword);
            if (player.wornHasKeyword(keyword)) {
              equippedWeapons.push(type.name);
              if (equippedWeapons.length == 2) {
                // put left hand weapon in left slot
                if (leftHandedWeapon?.hasKeyword(keyword)) {
                  equippedWeapons.reverse();
                }
                return;
              }
            }
          });
          if (
            equippedWeapons.length === 0 ||
            (equippedWeapons.length === 1 && ['shieldlight', 'shieldheavy', 'magicstaff'].includes(equippedWeapons[0]))
          ) {
            const clawRaces = [0x00013745, 0x00013740];
            const base = ctx.sp.ActorBase.from(player.getBaseObject()) as ActorBase;
            const raceId = base.getRace() ? (base.getRace() as Race).getFormID() : 0;
            if (clawRaces.includes(raceId)) {
              equippedWeapons.push('claw');
            } else {
              equippedWeapons.push('fist');
            }
          }
          armorKeywords.forEach((type) => {
            const keyword = ctx.sp.Keyword.getKeyword(type.keyword);
            if (player.wornHasKeyword(keyword)) {
              armorType = type.name;
            }
          });
          const src = `
          window.dispatchEvent(new CustomEvent('initSkillDices', { detail: { skills: ${eventString}, weapons: ${JSON.stringify(
            equippedWeapons
          )}, armor: ${JSON.stringify(armorType)}}}))
          `;
          ctx.sp.browser.executeJavaScript(src);
        },
        { eventString: JSON.stringify(possessedSkills), weaponKeywords: weaponTypes, armorKeywords: armorTypes }
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
          text: `${actorName} восстанавливает единицу здоровья - ❤${type}`,
          color: colors['green'],
          type: ['plain'],
        },
      ];
      break;
    case 'self-attack':
      text = [
        {
          text: `${actorName} теряет единицу здоровья - ❤${type}`,
          color: colors['red'],
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
      text = [];
      const magicBuff = +buff;
      if (magicBuff < 0) {
        text.push({
          text: `${actorName} под ослаблением. Сила воли снижена на `,
          color: colors['purple'],
          type: ['plain'],
        });
        text.push({
          text: `${Math.abs(magicBuff)}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      if (magicBuff > 0) {
        text.push({
          text: `${actorName} под усилением. Сила воли повышена на `,
          color: colors['purple'],
          type: ['plain'],
        });
        text.push({
          text: `${buff}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      text.push({
        text: `${actorName} использует магию ${magicNames[type] || ''} `,
        color: colors['purple'],
        type: ['plain'],
      });
      text.push({
        text: `- ${Math.floor(Math.random() * 20 + 1) + (+value + magicBuff)}`,
        color: colors['white'],
        type: ['plain'],
      });
      break;
    case 'weapon':
      const weaponNames = {
        daggers: 'кинжал',
        shortswords: 'короткий меч',
        swords: 'меч',
        scimitar: 'скимитар',
        katana: 'катану',
        mace: 'булаву',
        axes: 'топор',
        hammer: 'молоток',
        bows: 'лук или арбалет',
        longsword: 'двуручный меч',
        greatkatana: 'нодати',
        battleaxe: 'двуручный топор',
        warhammer: 'молот',
        staff: 'боевой посох',
        pike: 'копье',
        halberd: 'алебарду',
        fist: 'дерется в рукопашную',
        claw: 'когти',
        magicstaff: 'магический посох',
        different: 'парное оружие',
      } as { [key: string]: string };
      text = [];
      const attackBuff = +buff;
      if (attackBuff < 0) {
        text.push({
          text: `${actorName} под ослаблением. Сила атаки снижена на `,
          color: colors['blue'],
          type: ['plain'],
        });
        text.push({
          text: `${Math.abs(attackBuff)}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      if (attackBuff > 0) {
        text.push({
          text: `${actorName} под усилением. Сила атаки повышена на `,
          color: colors['blue'],
          type: ['plain'],
        });
        text.push({
          text: `${buff}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      text.push({
        text: `${actorName} ${type === 'fist' ? weaponNames[type] : `использует ${weaponNames[type]}`} `,
        color: colors['blue'],
        type: ['plain'],
      });
      text.push({
        text: `- ${Math.floor(Math.random() * 20 + 1) + (+value + attackBuff)}`,
        color: colors['white'],
        type: ['plain'],
      });
      break;
    case 'defence':
      text = [];
      const defenceBuff = +buff;
      if (defenceBuff < 0) {
        text.push({
          text: `${actorName} под ослаблением. Защита снижена на `,
          color: colors['yellow'],
          type: ['plain'],
        });
        text.push({
          text: `${Math.abs(defenceBuff)}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      if (defenceBuff > 0) {
        text.push({
          text: `${actorName} под усилением. Защита повышена на `,
          color: colors['yellow'],
          type: ['plain'],
        });
        text.push({
          text: `${buff}\n`,
          color: colors['white'],
          type: ['plain'],
        });
      }
      text.push({
        text: `${actorName} ❤${hitPoints} защищается `,
        color: colors['yellow'],
        type: ['plain'],
      });
      text.push({
        text: `- ${Math.floor(Math.random() * 20 + 1) + (+value + +buff)}`,
        color: colors['white'],
        type: ['plain'],
      });
      break;
    case 'wolf':
      if (type === 'on') {
        text = [{ text: `${actorName} превращается в вервольфа`, color: colors['blue'], type: ['plain'] }];
      } else {
        text = [{ text: `${actorName} возвращется в человеческую форму`, color: colors['blue'], type: ['plain'] }];
      }
      break;
    case 'vampus':
      if (type === 'on') {
        text = [
          { text: `Ночное время. ${actorName} использует вампирские навыки`, color: colors['purple'], type: ['plain'] },
        ];
      } else {
        text = [
          {
            text: `Взошло солнце. ${actorName} не может использовать вампирские навыки`,
            color: colors['purple'],
            type: ['plain'],
          },
        ];
      }
      break;
    default:
      break;
  }
  const message = new ChatMessage(actorId, masterApiId || controller.getProfileId(actorId), text, 'dice', controller);
  for (const neighbor of neighbors) {
    controller.sendChatMessage(neighbor, message);
  }
};

export const weaponTypes = [
  { keyword: 'WeapTypeDagger', name: 'daggers' },
  { keyword: 'WeapTypeShortSword', name: 'shortswords' },
  { keyword: 'WeapTypeScimitar', name: 'scimitar' },
  { keyword: 'WeapTypeSword', name: 'swords' },
  { keyword: 'WeapTypeKatana', name: 'katana' },
  { keyword: 'WeapTypeWarAxe', name: 'axes' },
  { keyword: 'WeapTypeClub', name: 'mace' },
  { keyword: 'WeapTypeMace', name: 'mace' },
  { keyword: 'WeapTypeHammer', name: 'hammer' },
  { keyword: 'WeapTypeGreatsword', name: 'longsword' },
  { keyword: 'WeapTypeGreatKatana', name: 'greatkatana' },
  { keyword: 'WeapTypeBattleaxe', name: 'battleaxe' },
  { keyword: 'WeapTypeWarhammer', name: 'warhammer' },
  { keyword: 'WeapTypePike', name: 'pike' },
  { keyword: 'WeapTypeGlaive', name: 'pike' },
  { keyword: 'WeapTypeHalberd', name: 'halberd' },
  { keyword: 'WeapTypeBattleStaff', name: 'staff' },
  { keyword: 'WeapTypeBow', name: 'bows' },
  { keyword: 'ArmorShieldLight', name: 'shieldlight' },
  { keyword: 'ArmorShieldHeavy', name: 'shieldheavy' },
];

export const armorTypes = [
  { keyword: 'ArmorLight', name: 'armorlight' },
  { keyword: 'ArmorHeavy', name: 'armorheavy' },
  { keyword: 'ArmorMageRobe', name: 'robe' },
];
