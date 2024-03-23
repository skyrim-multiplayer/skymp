import { Movement } from "../sync/movement";
import { Animation } from "../sync/animation";
import { Appearance } from "../sync/appearance";
import { Equipment } from "../sync/equipment";
import { Inventory } from "../sync/inventory";

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
  templateChain?: number[];
  lastAnimation?: string;

  // Assigned locally
  isMyClone?: boolean;
}

export interface WorldModel {
  forms: Array<FormModel | undefined>;
  playerCharacterFormIdx: number;
}
