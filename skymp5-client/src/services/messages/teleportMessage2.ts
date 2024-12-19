import { MsgType } from "../../messages";

export interface TeleportMessage2 {
    t: MsgType.Teleport2;
    pos: number[];
    rot: number[];
    worldOrCell: number;
}
