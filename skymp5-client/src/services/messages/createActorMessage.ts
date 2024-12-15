import { Appearance } from "../../sync/appearance";
import { Equipment } from "../../sync/equipment";
import { Inventory } from "../../sync/inventory";
import { Transform } from "../../sync/movement";
import { Animation } from "../../sync/animation";
import { MsgType } from "src/messages";

export interface CreateActorMessage extends CreateActorMessageMainProps {
    t: MsgType.CreateActor,
    idx: number;
    baseRecordType?: "DOOR"; // see PartOne.cpp
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

    isDead?: boolean;
}

export interface SetNodeTextureSetEntry {
    nodeName: string;
    textureSetId: number;
};

export interface SetNodeScaleEntry {
    nodeName: string;
    scale: number;
};

export interface CreateActorMessageAdditionalProps {
    isOpen?: boolean;
    isHarvested?: boolean;
    setNodeTextureSet?: SetNodeTextureSetEntry[];
    setNodeScale?: SetNodeScaleEntry[];
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
    inventory?: Inventory;

    isDead?: boolean; // TODO: take a look why doubles CreateActorMessageMainProps
}
