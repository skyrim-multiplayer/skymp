export interface ISkillDices {
  onClose: () => void;
  send: (msg: string) => void;
}

export interface IPossessedSkills {
  [key: string]: {
    id: number;
    level: number;
    price: number;
  }
}

export type IMagic =
  | 'conjuration'
  | 'destruction'
  | 'restoration'
  | 'alteration'
  | 'illusion'
  | null;

export type IWeapon = 'daggers' | 'shortswords' | 'swords' | 'scimitar' | 'katana' | 'mace' | 'axes' | 'hammer' | 'bows' | 'longsword' | 'greatkatana' | 'battleaxe' | 'warhammer' | 'staff' | 'pike' | 'halberd' | 'fist' | 'claw' | 'magicstaff' | 'different';
