import { MsgType } from "../../messages";

export interface TeleportMessage {
    t: MsgType.Teleport;
    idx: number;
    pos: number[];
    rot: number[];
    worldOrCell: number;
}
