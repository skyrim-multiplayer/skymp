import { MsgType } from "../../messages";

export interface HostStartMessage {
    t: MsgType.HostStart;
    target: number;
}
