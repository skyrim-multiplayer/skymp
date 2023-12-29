import { Appearance } from "../../sync/appearance";
import { Equipment } from "../../sync/equipment";
import { Inventory } from "../../sync/inventory";
import { Transform } from "../../sync/movement";

export interface CreateActorMessage {
    type: "createActor";
    idx: number;
    refrId?: number;
    baseRecordType: "DOOR" | undefined; // see PartOne.cpp
    transform: Transform;
    isMe: boolean;
    appearance?: Appearance;
    equipment?: Equipment;
    inventory?: Inventory;
    baseId?: number;
    props?: Record<string, unknown>;
}
