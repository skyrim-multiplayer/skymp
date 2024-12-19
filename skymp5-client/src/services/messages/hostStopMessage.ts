import { MsgType } from "../../messages";

export interface HostStopMessage {
    t: MsgType.HostStop;
    target: number;
}
