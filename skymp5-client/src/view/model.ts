import {
  CreateActorMessageAdditionalProps,
  CreateActorMessageMainProps,
} from 'src/services/messages/createActorMessage';

import { Animation } from '../sync/animation';
import { Appearance } from '../sync/appearance';
import { Equipment } from '../sync/equipment';
import { Inventory } from '../sync/inventory';
import { Movement } from '../sync/movement';

// Own properties (not inherited) are being assigned locally
export interface FormModel
  extends CreateActorMessageAdditionalProps,
    CreateActorMessageMainProps {
  idx?: number;
  movement?: Movement;
  numMovementChanges?: number;
  numAppearanceChanges?: number;
  isMyClone?: boolean;
}

export interface WorldModel {
  forms: Array<FormModel | undefined>;
  playerCharacterFormIdx: number;
  playerCharacterRefrId: number;
}
