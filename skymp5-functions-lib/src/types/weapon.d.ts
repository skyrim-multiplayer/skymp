import { Form } from './form';

export class Weapon extends Form {
  constructor(formId: number) {}
  GetBaseDamage(): number | undefined;
}

export enum WeaponType {
  Fists,
  Swords,
  Daggers,
  WarAxes,
  Maces,
  Greatswords,
  BattleaxesANDWarhammers,
  Bows,
  Staff,
  Crossbows,
}
