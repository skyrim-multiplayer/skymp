import { Appearance } from "../../sync/appearance";
import { Equipment } from "../../sync/equipment";
import { Inventory } from "../../sync/inventory";
import { Transform } from "../../sync/movement";
import { Animation } from "../../sync/animation";

export interface CreateActorMessage extends CreateActorMessageMainProps {
    type: "createActor";
    idx: number;
    baseRecordType: "DOOR" | undefined; // see PartOne.cpp
    transform: Transform;
    isMe: boolean;
    props?: CreateActorMessageAdditionalProps;
}

export interface CreateActorMessageMainProps {
    refrId?: number;
    baseId?: number;
    appearance?: Appearance;
    equipment?: Equipment;
    animation?: Animation;

    inventory?: Inventory;
    isDead?: boolean;
}

export interface CreateActorMessageAdditionalProps {
    isOpen?: boolean;
    isHarvested?: boolean;
    setNodeTextureSet?: Record<string, number>;
    setNodeScale?: Record<string, number>;
    disabled?: boolean;
    lastAnimation?: string;
    displayName?: string;
    isHostedByOther?: boolean;
    isRaceMenuOpen?: boolean;
    learnedSpells?: number[];
    healRate?: number;
    healRateMult?: number;
    health?: number;
    magickaRate?: number;
    magickaRateMult?: number;
    magicka?: number;
    staminaRate?: number;
    staminaRateMult?: number;
    stamina?: number;
    healthPercentage?: number;
    staminaPercentage?: number;
    magickaPercentage?: number;
    templateChain?: number[];

    inventory?: Inventory; // TODO: take a look why doubles CreateActorMessageMainProps
    isDead?: boolean; // TODO: take a look why doubles CreateActorMessageMainProps
}
