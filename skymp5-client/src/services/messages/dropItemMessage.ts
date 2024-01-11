import { MsgType } from "../../messages";

export interface DropItemMessage {
    t: MsgType.DropItem,
    baseId: number,
    count: number
}
