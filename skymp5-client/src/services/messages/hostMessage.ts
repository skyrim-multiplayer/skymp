import { MsgType } from "../../messages";

export interface HostMessage {
    t: MsgType.Host,
    remoteId: number
}
