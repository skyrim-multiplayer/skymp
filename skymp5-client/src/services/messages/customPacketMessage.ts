import { MsgType } from "../../messages";

export interface CustomPacketMessage {
    t: MsgType.CustomPacket,
    contentJsonDump: string
}
