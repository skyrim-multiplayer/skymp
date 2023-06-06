export interface ISkillDices {
  onClose: () => void;
  send: (msg: string) => boolean;
  disableSound: boolean;
}

export interface IPossessedSkills {
  [key: string]: {
    id: number;
    level: number;
    price: number;
  }
}

export type IRollAction = 'initiative' | 'weapon' | 'magic' | 'defence';

export type IMagic =
  | 'conjuration'
  | 'destruction'
  | 'restoration'
  | 'alteration'
  | 'illusion';

export type IWeapon = 'daggers' | 'shortswords' | 'swords' | 'scimitar' | 'katana' | 'mace' | 'axes' | 'hammer' | 'bows' | 'longsword' | 'greatkatana' | 'battleaxe' | 'warhammer' | 'staff' | 'pike' | 'halberd' | 'fist' | 'claw' | 'magicstaff' | 'different' | 'shieldlight' | 'shieldheavy';

export type IDefence = 'armorlight' | 'armorheavy' | 'robe';

export interface ISkillDicesData {
  skills: IPossessedSkills;
  weapons: IWeapon[];
  armor: IDefence;
};
