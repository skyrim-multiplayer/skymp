import { MsgType } from "../../messages";

export interface ActivateMessage {
    t: MsgType.Activate,
    data: {
        caster: number,
        target: number
    }
}
