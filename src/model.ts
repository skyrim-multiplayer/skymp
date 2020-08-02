import { Movement } from "./components/movement";
import { Animation } from "./components/animation";
import { Look } from "./components/look";
import { Equipment } from "./components/equipment";

export interface FormModel {
  baseId?: number;
  movement?: Movement;
  animation?: Animation;
  numMovementChanges?: number;
  look?: Look;
  equipment?: Equipment;
}

export interface WorldModel {
  forms: FormModel[];
  playerCharacterFormIdx: number;
}
