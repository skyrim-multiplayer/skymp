import { Animation } from '../sync/animation';
import { Appearance } from '../sync/appearance';
import { Equipment } from '../sync/equipment';
import { Inventory } from '../sync/inventory';
import { Movement } from '../sync/movement';

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

  // Assigned locally
  isMyClone?: boolean;
}

export interface WorldModel {
  forms: FormModel[];
  playerCharacterFormIdx: number;
}
