import { MsgType } from "../../messages";

export interface CustomEventMessage {
    t: MsgType.CustomEvent,
    argsJsonDumps: string[],
    eventName: string
}
