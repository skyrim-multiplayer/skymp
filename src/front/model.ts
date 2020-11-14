import { Movement } from "./components/movement";
import { Animation } from "./components/animation";
import { Look } from "./components/look";
import { Equipment } from "./components/equipment";
import { Inventory } from "./components/inventory";

export interface FormModel {
  baseId?: number;
  refrId?: number;
  movement?: Movement;
  animation?: Animation;
  numMovementChanges?: number;
  look?: Look;
  numLookChanges?: number;
  equipment?: Equipment;
  isHarvested?: boolean;
  isOpen?: boolean;
  inventory?: Inventory;
  isHostedByOther?: boolean;
  isHostedByMe?: boolean;
}

export interface WorldModel {
  forms: FormModel[];
  playerCharacterFormIdx: number;
}
