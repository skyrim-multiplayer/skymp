import { MsgType } from "../../messages";

export interface ActivateMessage {
    t: MsgType.Activate,
    data: ActivateMessageData
}

interface ActivateMessageData {
    caster: number;
    target: number;
    isSecondActivation: boolean;
}
