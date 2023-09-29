import { MsgType } from "../../messages";

export interface CustomPacketMessage {
    t: MsgType.CustomPacket,
    content: {
        customPacketType: string,
        gameData: Record<string, unknown>
    }
}
