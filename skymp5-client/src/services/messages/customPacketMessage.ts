import { MsgType } from "../../messages";

export interface CustomPacketMessage {
    t: MsgType.CustomPacket,
    contentJsonDump: string
}

interface InvokeAnimResult {
    customPacketType: "invokeAnimResult",
    result: {
        success: boolean,
        reason?: string
    },
    requestId?: string | number;
}
