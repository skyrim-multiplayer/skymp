import { Movement } from './components/movement';
import { Animation } from './components/animation';
import { Look } from './components/look';

export interface FormModel {
    baseId?: number;
    movement?: Movement;
    animation?: Animation;
    numMovementChanges?: number;
    look?: Look;
}

export interface WorldModel {
    forms: FormModel[];
    playerCharacterFormIdx: number;
}