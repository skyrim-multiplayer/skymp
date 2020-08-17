import { Movement } from "./components/movement";
import { Animation } from "./components/animation";
import { Look } from "./components/look";
import { Equipment } from "./components/equipment";

export interface FormModel {
  baseId?: number;
  refrId?: number;
  movement?: Movement;
  animation?: Animation;
  numMovementChanges?: number;
  look?: Look;
  equipment?: Equipment;
  isHarvested?: boolean;
  isOpen?: boolean;
}

export interface WorldModel {
  forms: FormModel[];
  playerCharacterFormIdx: number;
}
