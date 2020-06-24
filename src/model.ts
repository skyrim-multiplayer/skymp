import { Movement } from './components/movement';
import { Animation } from './components/animation';

export interface FormModel {
    baseId?: number;
    movement?: Movement;
    animation?: Animation;
}

export interface WorldModel {
    forms: FormModel[];
    playerCharacterFormIdx: number;
}