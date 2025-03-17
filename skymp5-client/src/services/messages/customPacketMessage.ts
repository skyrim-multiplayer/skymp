import { MsgType } from "../../messages";

export interface CustomPacketMessage {
    t: MsgType.CustomPacket,
    content: LoginWithSkympIo | InvokeAnimResult
}

interface LoginWithSkympIo {
    customPacketType: "loginWithSkympIo",
    gameData: Record<string, unknown>
}

interface InvokeAnimResult {
    customPacketType: "invokeAnimResult",
    result: {
        success: boolean,
        reason?: string
    },
    requestId?: string | number;
}
