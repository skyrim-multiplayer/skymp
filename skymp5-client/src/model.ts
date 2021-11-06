import { Movement } from "./movement";
import { Animation } from "./animation";
import { Appearance } from "./appearance";
import { Equipment } from "./equipment";
import { Inventory } from "./inventory";

export interface FormModel {
  idx?: number;
  baseId?: number;
  refrId?: number;
  movement?: Movement;
  animation?: Animation;
  numMovementChanges?: number;
  appearance?: Appearance;
  numAppearanceChanges?: number;
  equipment?: Equipment;
  isHarvested?: boolean;
  isOpen?: boolean;
  inventory?: Inventory;
  isHostedByOther?: boolean;
  isDead?: boolean;
}

export interface WorldModel {
  forms: FormModel[];
  playerCharacterFormIdx: number;
}
