import { MsgType } from "../../messages";

// export interface CustomPacketMessage {
//     t: MsgType.CustomPacket,
//     content: CustomPacketMessageContent
// }

// interface CustomPacketMessageContent {
//     customPacketType: string,
//     gameData: Record<string, unknown>
// }

export interface CustomPacketMessage {
    t: MsgType.CustomPacket,
    contentJsonDump: string
}
