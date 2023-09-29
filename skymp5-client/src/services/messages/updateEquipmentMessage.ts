import { MsgType } from "../../messages";
import { Equipment } from "../../sync/equipment";

export interface UpdateEquipmentMessage {
    t: MsgType.UpdateEquipment;
    idx: number;
    data: Equipment;
}
