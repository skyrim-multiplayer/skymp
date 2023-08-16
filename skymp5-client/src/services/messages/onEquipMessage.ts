import { MsgType } from "../../messages";

export interface OnEquipMessage {
    t: MsgType.OnEquip,
    baseId: number
}
